/*  PCO : labo 04 - Tri à bulles
 *  Fichier : ThreadSorter.h
 *  Auteurs : Mélanie Huck - James Nolan
 *  29.05.2015
 */
#ifndef THREADSORTER_H
#define THREADSORTER_H

#include <QThread>
#include <QMutex>
#include <QSemaphore>
#include "bubblesortthreaded.h"

template<typename T>
class BubbleSortThreaded;

template<typename T>
class BThread : public QThread
{
    // La classe du "contrôleur" est déclarée "amie" pour qu'il puisse définir les paramètres
    // de ce thread plus facilement. Par exemple, le thread risque de se mettre en attente passive
    // des modifications de ses voisins alors que tous ses voisins ont terminé. C'est le
    // contrôleur qui doit le réveiller dans ce cas, et il a besoin d'accéder à son mutex d'attente
    friend class BubbleSortThreaded<T>;

private:
    // Le thread possède un pointeur vers le début de la zone qu'il doit trier au sein du tableau.
    // La taille de sa zone lui est aussi donnée
    T *_a;
    size_t _size;

    // Le thread doit connaître les zones contigües (si elles existent), pour pouvoir interagi avec
    // lorsqu'il modifie un de leurs cases
    BThread *_nextZone;
    BThread *_prevZone;

    // Indique si la première case, respectivement la dernière case du tableau a été modifiée par
    // le thread voisin
    bool _firstValUpdated;
    bool _lastValUpdated;

    // L'accès à ces cases sensibles est géré par deux mutexes. Les threads des zones contigues
    // utilisent ces mêmes mutex, ce qui permet de ne pas autoriser les deux threads à accéder
    // à ces variables partagées en même temps. Notons que ces mutexes sont utilisé pour protéger les
    // booléens ci-dessus ainsi l'accès aux cases
    QMutex *_prevSharedMutex;
    QMutex *_nextSharedMutex;

    // Pour ménager le processeur, le thread est en attente passive sur une sémpaphore lorsqu'il
    // n'est entrain de trier. C'est les threads voisins qui libèrent cette sémpaphore s'ils
    // ont modifié l'état de sa zone. Une variable booléenne permet de connaître cet état
    bool _sleeps;
    QSemaphore _sleep;

    // Le thread possède une référence vers le contrôleur pour pouvoir lui indiquer l'état de son
    // tri. Le contrôleur se chargera alors de lister toutes les zones triées.
    BubbleSortThreaded<T> *_controler;

public:
    // Le constructeur initialise bêtement tous les attributs
    BThread(BubbleSortThreaded<T> *sorter, T *tabSlice, size_t tabSize, QMutex *firstLock, QMutex *lastLock) : QThread(),
    _controler(sorter), _firstValUpdated(false), _lastValUpdated(false), _sleeps(false), _nextSharedMutex(lastLock), _prevSharedMutex(firstLock),
    _a(tabSlice), _size(tabSize), _sleep(0) { }

    // La méthode run est exécutée avec le thread
    void run() {


        // Le thread trie son tableau tant que le tri global n'est pas terminé
        while (!_controler->isFinished()) {

            // On passe enfin au bubble sort (inspiré du bubblesort fournit dans la donnée)
            if (_size == 1) {
                // Heureusement, on ne doit pas trier un tableau d'un seul élément
            }else if (_size == 2) {
                // Dans le cas d'un tableau à deux éléments, il nous faut faire un seul swap
                // mais en tenant compte de la concurrence. Il faut tenir co
                T swap;
                if (_nextZone != nullptr) {
                    _nextSharedMutex->lock();
                }
                if (_prevZone != nullptr) {
                    _prevSharedMutex->lock();
                }
                if (_a[0] > _a[1]) {
                    swap    = _a[0];
                    _a[1]   = _a[1];
                    _a[0] = swap;

                    // S'il y a eu changement notifier les voisins
                    if (_nextZone != nullptr) {
                        _nextZone->_firstValUpdated = true;

                        if (_nextZone->_sleeps) {
                            _controler->setSortState(false); // indication au contrôleur que ce n'est pas fini
                            _nextZone->_sleeps = false;
                            _nextZone->_sleep.release();     // réveil le précédent
                        }
                    }
                    if (_prevZone != nullptr) {
                        _prevZone->_lastValUpdated = true;

                        if (_prevZone->_sleeps) {
                            _controler->setSortState(false); // indication au contrôleur que ce n'est pas fini
                            _prevZone->_sleeps = false;
                            _prevZone->_sleep.release();     // réveil le précédent
                        }
                    }
                }

                // Libérer les locks éventuellement pris
                if (_nextZone != nullptr) {
                    _nextSharedMutex->unlock();
                }
                if (_prevZone != nullptr) {
                    _prevSharedMutex->unlock();
                }
            }

            if (_size > 1) {
                T swap; // Variable temporaire pour échanger deux valeurs
                for (int c = _size - 1 ; c > 0; --c) {
                    // Si la dernière case de la zone a été modifiée par le thread de la
                    // zone suivante
                    if (_nextZone != nullptr && _lastValUpdated) {
                        // On réinitialise le flag...
                        _nextSharedMutex->lock();
                        _lastValUpdated = false;
                        _nextSharedMutex->unlock();

                        // ...  et on recommence le tri. Ca ne sert à rien de trier la
                        // suite du tableau si le haut n'est pa consistant
                        c = _size - 1;
                    }

                    for (int d = 0 ; d < c; ++d) {
                        // Avant de lire ou de modifier quoique ce soit, il faut s'assurer
                        // que l'on ne touche pas à une case sensible
                        if (_prevZone != nullptr && d == 0) {
                            // Dans le cas où nous toucherions une case partagée avec la zone
                            // inféreure, en bloquer l'accès
                            _prevSharedMutex->lock();

                            // Echanger les valeurs des cases si elles ne sont pas dans le bon ordre
                            if (_a[d] > _a[d+1]) {
                                swap    = _a[d];
                                _a[d]   = _a[d+1];
                                _a[d+1] = swap;

                                // Avertir la zone précédente que l'on a modifié un de ses valeurs
                                // Il faut peut-être réveille cette zone qui ne sait pas encore que
                                // quelque chose a changé
                                _prevZone->_lastValUpdated = true;     // indication du changement


                                if (_prevZone->_sleeps) {
                                    _controler->setSortState(false); // indication au contrôleur que ce n'est pas fini
                                    _prevZone->_sleeps = false;
                                    _prevZone->_sleep.release();     // réveil le précédent
                                }
                            }

                            // Dévérouiller l'accès à la case
                            _prevSharedMutex->unlock();
                        }
                        else if (_nextZone != nullptr && c == _size - 1) {
                            // Dans le cas où nous toucherions une case partagée avec la zone
                            // supérieure, en bloquer l'accès
                            _nextSharedMutex->lock();

                            // Echanger les valeurs des cases si elles ne sont pas dans le bon ordre
                            if (_a[d] > _a[d+1]) {
                                swap    = _a[d];
                                _a[d]   = _a[d+1];
                                _a[d+1] = swap;

                                // Avertir la zone suivante que l'on a modifié un de ses valeurs
                                // Il faut peut-être réveille cette zone qui ne sait pas encore que
                                // quelque chose a changé
                                _nextZone->_firstValUpdated = true;     // indication du changement

                                if (_nextZone->_sleeps) {
                                    _controler->setSortState(false); // indication au contrôleur que ce n'est pas fini
                                    _nextZone->_sleeps = false;
                                    _nextZone->_sleep.release();     // réveil le précédent
                                }
                            }

                            // Dévérouiller l'accès à la case
                            _nextSharedMutex->unlock();
                        } else {
                            // Sinon c'est le cas standard, aucun risque d'un point de vue concurrent
                            if (_a[d] > _a[d+1]) {
                                swap    = _a[d];
                                _a[d]   = _a[d+1];
                                _a[d+1] = swap;
                            }
                        }
                    }
                }
            }

            // Le tri est effectué. Avant de "s'endormir", le thread doit vérifier que ses threads
            // voisins n'ont pas modifié sa zone depuis la fin de son tri
            if (_prevZone != nullptr) {
                _prevSharedMutex->lock();
            }
            if (_nextZone != nullptr) {
                _nextSharedMutex->lock();
            }

            // Si tout est encore en ordre
            if (!_firstValUpdated && !_lastValUpdated) {
                // Le thread peut s'endormir sereinement jusqu'à ce que le contrôleur
                // ou l'un de ses voisins le réveil
                _sleeps = true;
                if (_prevZone != nullptr) {
                    _prevSharedMutex->unlock();
                }
                if (_nextZone != nullptr) {
                    _nextSharedMutex->unlock();
                }

                // Indiquer au contrôleur que tout est bon dans cette zone
                _controler->setSortState(true);

                // S'endormir
                _sleep.acquire();
            } else {
                // Si par contre la dernière ou la première case a été mise à jour,
                // Recommencer le tri depuis le début
                _lastValUpdated = _firstValUpdated = false;
                if (_nextZone != nullptr) {
                    _nextSharedMutex->unlock();
                }
                if (_prevZone != nullptr) {
                    _prevSharedMutex->unlock();
                }
            }
        }
    }
};

#endif // THREADSORTER_H
