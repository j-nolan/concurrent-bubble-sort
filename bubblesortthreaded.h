/*  PCO : labo 04 - Tri à bulles
 *  Fichier : BubbleSortThreaded.h
 *  Auteurs : Mélanie Huck - James Nolan
 *  29.05.2015
 */
#ifndef BUBBLESORTTHREADED_H
#define BUBBLESORTTHREADED_H

#include <QMutex>
#include <QSemaphore>

#include "isort.h"
#include "bthread.h"

template<typename T>
class BubbleSortThreaded : public ISort<T> {
private:
    // Ce mutex sera initialement vérouillé, et dévérouillé uniquement quand
    // toutes les zones auront été triées. On garde aussi un booléen qui contient
    // cette information
    QMutex _finished;
    bool _isFinished;


    // Le nombre de zones actuellement triées. Cette valeur permettra de détecter
    // la fin du tri global. Comme plusieurs threads sont susceptibles en même temps
    // de vouloir indiquer qu'ils ont terminé, on protège cette variable par
    // un mutex spécifique
    quint64 _nbSortedZonesMutex;
    QMutex  _nbSortedZones;

    // Les threads qui trieront le tableau. Le tableau est partagé en
    // plusieurs zones, chacune étant allouée à un thread. Les extrêmités
    // de chaque zones sont partagées entre deux threads, on associe donc pour
    // chacune de ces extrêmités un thread permettant d'en protéger l'accès
    // parallèle
    const quint64 _nbThreads;
    BThread<T>   **_threads;
    QMutex       **_contiguousValuesMutexes;

public:

    // Le constructeur initialise les attributs dont il peut déjà connaître la valeur
    BubbleSortThreaded(quint64 nbThread) : _nbSortedZonesMutex(0), _nbThreads(nbThread),
        _contiguousValuesMutexes(new QMutex*[nbThread - 1]), _threads(new BThread<T>*[nbThread])
    {
        // A l'initialisation, le mutex indiquant si tous les tris sont terminés est
        // bloqué. Il ne sera débloqué que lorsque tous les threads auront indiqué
        // qu'ils sont triés
        _finished.lock();

        // Initialier les mutexes partagés
        for (quint64 i = 0; i < _nbThreads - 1; ++i) {
            _contiguousValuesMutexes[i] = new QMutex();
        }
    }

    ~BubbleSortThreaded() {

        // Supprimer les mutexes partagés
        for (quint64 i = 0; i < _nbThreads - 1; ++i) {
            delete _contiguousValuesMutexes[i];
        }
        delete _contiguousValuesMutexes;
    }


    // Méthode permettant générale du tri. Lorsqu'elle est appelée, le nombre
    // de cases du tableau est découpé en plusieurs zones, dont le nombre dépend
    // du nombre de threads désirés (cf. constructeur)
    void sort(T a[], qint64 size)
    {

        // ------------------------------------------------------------------
        // Découper le tableau en tranches (quasiment) égales
        quint64 _sliceSize = size / _nbThreads;
        quint64 _remainder = size % _nbThreads;

        // Créer les threads en indiquant la tranche assignée à chaque thread
        // S'il y a un reste,  le répartir entre les différentes tranches
        quint64 _sliceBeg = 0;                           // y compris
        quint64 _sliceEnd = _sliceBeg + _sliceSize - 1;  // y compris

        // Création des threads
        for (int i = 0; i < _nbThreads; ++i) {

            // Les mutexes que l'on passera aux threads pour vérouiller sur le premier et
            // dernier élément
            QMutex *firstElementMutex, *lastElementMutex;

            // S'il reste des cases à distribuer à cause de la division entière
            while (_remainder != 0) {
                // Ajouter une case à cette zone
                ++_sliceEnd;
                firstElementMutex = (i != 0) ? _contiguousValuesMutexes[i - 1] : nullptr;
                lastElementMutex = (i != _nbThreads - 1) ? _contiguousValuesMutexes[i] : nullptr;

                _threads[i] = new BThread<T>(this, a + _sliceBeg, _sliceEnd - _sliceBeg + 1, firstElementMutex, lastElementMutex);
                _sliceBeg = _sliceEnd;
                _sliceEnd += _sliceSize;
                --_remainder;
                ++i;
            }

            firstElementMutex = (i != 0) ? _contiguousValuesMutexes[i - 1] : nullptr;
            lastElementMutex = (i != _nbThreads - 1) ? _contiguousValuesMutexes[i] : nullptr;

            _threads[i] = new BThread<T>(this, a + _sliceBeg, _sliceEnd - _sliceBeg + 1, firstElementMutex, lastElementMutex);
            _sliceBeg = _sliceEnd;
            _sliceEnd += _sliceSize;

        }

        // Maintenant que tous les threads sont créés, il faut leur indiquer quels sont leurs
        // zones contigues respectives pour qu'ils puissent interagir avec celles-ci si nécessare
        for (quint16 i = 0; i < _nbThreads; ++i) {
            // Le premier thread n'a pas de précédésseur
            _threads[i]->_prevZone = (i != 0) ? _threads[i - 1] : nullptr;

            // Le dernier thread n'a pas de successeur
             _threads[i]->_nextZone = (i != _nbThreads - 1) ? _threads[i + 1] : nullptr;
        }

        // Tous les threads sont prêts, ont les lance !
        for (quint16 i = 0; i < _nbThreads; ++i) {
            _threads[i]->start();
        }

        // On attend sur le mutex du tri général. Ce mutex sera débloqué lorsque tous les threads
        // auront indiqué qu'ils sont trié
        _finished.lock();

        // Quand nous arrivons ici, tous les threads sont triés.
        for (quint16 i = 0; i < _nbThreads; ++i) {
            // Certains threads peuvent faire de l'attente passive en attendant d'être réveillé
            // par leurs voisins. Ces threads sont figés, il faut donc manuellement les forcer
            // à se réveiller pour qu'ils sachent que le tri est terminé
            _threads[i]->_sleep.release();
            _threads[i]->wait();
        }
    }

    // Méthode permettant à un thread d'indiquer qu'il a terminé ou qu'il va commencer à trier
    // sa zone
    void setSortState(bool sorted) {
        _nbSortedZones.lock();

        if (sorted) {
            // Si ce thread était le dernier sur lequel on attendait pour déclarer le tableau
            // trié
            if (++_nbSortedZonesMutex == _nbThreads) {
                // On indique que le tri global est effectué
                _isFinished = true;

                // On peut faire avancer le programme principal qui attendait sur cet événement
                _finished.unlock();
            }
        } else {
            // On ne considère plus cette zone comme triée
            _nbSortedZonesMutex--;
        }

        _nbSortedZones.unlock();
    }

    // Méthode indiquant si le tri global est terminé. Dans ce projet, elle est appelée par
    // les threads pour savoir s'ils doivent continuer à s'exécuter ou s'ils peuvent se
    // terminer
    bool isFinished() const {
        return _isFinished;
    }

};

// Indiquer au compilateur que BThread est accepté comme valeur templatée
template<typename T>
class BThread;

#endif // BUBBLESORTTHREADED_H
