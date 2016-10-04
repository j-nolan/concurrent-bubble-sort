/*  PCO : labo 04 - Tri à bulles
 *  Fichier : SortTester.cpp
 *  Auteurs : Mélanie Huck - James Nolan
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

    // Taille du tableau à donner en ligne de commande
    qint64 _tabSize;
    std::cout << "Entrer la taille du tableau desiree : ";
    std::cin >> _tabSize;

    // TODO vérification du nombre de threads entrés par rapport à la taille
    // du tableau
    // Nombre de threads pour le tri à bulles threadés
    quint64 _nbThreads;

    do {
        std::cout << "Entrer le nombre de threads desires : ";
        std::cin >> _nbThreads;
    } while (_nbThreads > _tabSize);

    srand(time(0)); // Pour la génération des nombres aléatoires

    // Tableau à trier
    int* _tab = new int[_tabSize];

    // Remplir le tableau d'entiers aléatoires
    std::cout << "Tableau original : " << std::endl;
    for (qint64 i = 0; i < _tabSize; i++) {
        _tab[i] = rand();
        std::cout << _tab[i] << std::endl;
    }

    // -----------------------------------------------------
    // Trier le tableau avec le tri à bulles threadé

     BubbleSortThreaded<int> sorterThreaded(_nbThreads);
     sorterThreaded.sort(_tab, _tabSize);

    // -----------------------------------------------------
    // Vérification du tri !

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
