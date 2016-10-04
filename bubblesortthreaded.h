/*  PCO : labo 04 - Tri � bulles
 *  Fichier : BubbleSortThreaded.h
 *  Auteurs : M�lanie Huck - James Nolan
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
    // Ce mutex sera initialement v�rouill�, et d�v�rouill� uniquement quand
    // toutes les zones auront �t� tri�es. On garde aussi un bool�en qui contient
    // cette information
    QMutex _finished;
    bool _isFinished;


    // Le nombre de zones actuellement tri�es. Cette valeur permettra de d�tecter
    // la fin du tri global. Comme plusieurs threads sont susceptibles en m�me temps
    // de vouloir indiquer qu'ils ont termin�, on prot�ge cette variable par
    // un mutex sp�cifique
    quint64 _nbSortedZonesMutex;
    QMutex  _nbSortedZones;

    // Les threads qui trieront le tableau. Le tableau est partag� en
    // plusieurs zones, chacune �tant allou�e � un thread. Les extr�mit�s
    // de chaque zones sont partag�es entre deux threads, on associe donc pour
    // chacune de ces extr�mit�s un thread permettant d'en prot�ger l'acc�s
    // parall�le
    const quint64 _nbThreads;
    BThread<T>   **_threads;
    QMutex       **_contiguousValuesMutexes;

public:

    // Le constructeur initialise les attributs dont il peut d�j� conna�tre la valeur
    BubbleSortThreaded(quint64 nbThread) : _nbSortedZonesMutex(0), _nbThreads(nbThread),
        _contiguousValuesMutexes(new QMutex*[nbThread - 1]), _threads(new BThread<T>*[nbThread])
    {
        // A l'initialisation, le mutex indiquant si tous les tris sont termin�s est
        // bloqu�. Il ne sera d�bloqu� que lorsque tous les threads auront indiqu�
        // qu'ils sont tri�s
        _finished.lock();

        // Initialier les mutexes partag�s
        for (quint64 i = 0; i < _nbThreads - 1; ++i) {
            _contiguousValuesMutexes[i] = new QMutex();
        }
    }

    ~BubbleSortThreaded() {

        // Supprimer les mutexes partag�s
        for (quint64 i = 0; i < _nbThreads - 1; ++i) {
            delete _contiguousValuesMutexes[i];
        }
        delete _contiguousValuesMutexes;
    }


    // M�thode permettant g�n�rale du tri. Lorsqu'elle est appel�e, le nombre
    // de cases du tableau est d�coup� en plusieurs zones, dont le nombre d�pend
    // du nombre de threads d�sir�s (cf. constructeur)
    void sort(T a[], qint64 size)
    {

        // ------------------------------------------------------------------
        // D�couper le tableau en tranches (quasiment) �gales
        quint64 _sliceSize = size / _nbThreads;
        quint64 _remainder = size % _nbThreads;

        // Cr�er les threads en indiquant la tranche assign�e � chaque thread
        // S'il y a un reste,  le r�partir entre les diff�rentes tranches
        quint64 _sliceBeg = 0;                           // y compris
        quint64 _sliceEnd = _sliceBeg + _sliceSize - 1;  // y compris

        // Cr�ation des threads
        for (int i = 0; i < _nbThreads; ++i) {

            // Les mutexes que l'on passera aux threads pour v�rouiller sur le premier et
            // dernier �l�ment
            QMutex *firstElementMutex, *lastElementMutex;

            // S'il reste des cases � distribuer � cause de la division enti�re
            while (_remainder != 0) {
                // Ajouter une case � cette zone
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

        // Maintenant que tous les threads sont cr��s, il faut leur indiquer quels sont leurs
        // zones contigues respectives pour qu'ils puissent interagir avec celles-ci si n�cessare
        for (quint16 i = 0; i < _nbThreads; ++i) {
            // Le premier thread n'a pas de pr�c�d�sseur
            _threads[i]->_prevZone = (i != 0) ? _threads[i - 1] : nullptr;

            // Le dernier thread n'a pas de successeur
             _threads[i]->_nextZone = (i != _nbThreads - 1) ? _threads[i + 1] : nullptr;
        }

        // Tous les threads sont pr�ts, ont les lance !
        for (quint16 i = 0; i < _nbThreads; ++i) {
            _threads[i]->start();
        }

        // On attend sur le mutex du tri g�n�ral. Ce mutex sera d�bloqu� lorsque tous les threads
        // auront indiqu� qu'ils sont tri�
        _finished.lock();

        // Quand nous arrivons ici, tous les threads sont tri�s.
        for (quint16 i = 0; i < _nbThreads; ++i) {
            // Certains threads peuvent faire de l'attente passive en attendant d'�tre r�veill�
            // par leurs voisins. Ces threads sont fig�s, il faut donc manuellement les forcer
            // � se r�veiller pour qu'ils sachent que le tri est termin�
            _threads[i]->_sleep.release();
            _threads[i]->wait();
        }
    }

    // M�thode permettant � un thread d'indiquer qu'il a termin� ou qu'il va commencer � trier
    // sa zone
    void setSortState(bool sorted) {
        _nbSortedZones.lock();

        if (sorted) {
            // Si ce thread �tait le dernier sur lequel on attendait pour d�clarer le tableau
            // tri�
            if (++_nbSortedZonesMutex == _nbThreads) {
                // On indique que le tri global est effectu�
                _isFinished = true;

                // On peut faire avancer le programme principal qui attendait sur cet �v�nement
                _finished.unlock();
            }
        } else {
            // On ne consid�re plus cette zone comme tri�e
            _nbSortedZonesMutex--;
        }

        _nbSortedZones.unlock();
    }

    // M�thode indiquant si le tri global est termin�. Dans ce projet, elle est appel�e par
    // les threads pour savoir s'ils doivent continuer � s'ex�cuter ou s'ils peuvent se
    // terminer
    bool isFinished() const {
        return _isFinished;
    }

};

// Indiquer au compilateur que BThread est accept� comme valeur templat�e
template<typename T>
class BThread;

#endif // BUBBLESORTTHREADED_H
