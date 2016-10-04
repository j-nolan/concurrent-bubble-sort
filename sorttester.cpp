/*  PCO : labo 04 - Tri � bulles
 *  Fichier : SortTester.cpp
 *  Auteurs : M�lanie Huck - James Nolan
 *  29.05.2015
 */

#include <QCoreApplication>
#include <iostream>
#include <time.h>

#include "bubblesort.h"
#include "bubblesortthreaded.h"
#include "sorttester.h"


SortTester::SortTester(){}

void SortTester::test()
{
    // -----------------------------------------------------
    // Initialisation

    // Taille du tableau � donner en ligne de commande
    qint64 _tabSize;
    std::cout << "Entrer la taille du tableau desiree : ";
    std::cin >> _tabSize;

    // TODO v�rification du nombre de threads entr�s par rapport � la taille
    // du tableau
    // Nombre de threads pour le tri � bulles thread�s
    quint64 _nbThreads;

    do {
        std::cout << "Entrer le nombre de threads desires : ";
        std::cin >> _nbThreads;
    } while (_nbThreads > _tabSize);

    srand(time(0)); // Pour la g�n�ration des nombres al�atoires

    // Tableau � trier
    int* _tab = new int[_tabSize];

    // Remplir le tableau d'entiers al�atoires
    std::cout << "Tableau original : " << std::endl;
    for (qint64 i = 0; i < _tabSize; i++) {
        _tab[i] = rand();
        std::cout << _tab[i] << std::endl;
    }

    // -----------------------------------------------------
    // Trier le tableau avec le tri � bulles thread�

     BubbleSortThreaded<int> sorterThreaded(_nbThreads);
     sorterThreaded.sort(_tab, _tabSize);

    // -----------------------------------------------------
    // V�rification du tri !

    int initial = _tab[0];
    bool error  = false;

    for (qint64  i = 0; i < _tabSize; i++) {
        std::cout << _tab[i] << std::endl;
        if(initial > _tab[i]) {
            error = true;
            break;
        }
        initial = _tab[i];
    }

    if (error)
       std::cout << "ERREUR " << std::endl;
    else
       std::cout << "Tri valide !" << std::endl;

    delete[] _tab;
}
