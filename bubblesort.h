#ifndef BUBBLESORT_H
#define BUBBLESORT_H

#include <isort.h>


template<typename T>
class BubbleSort : public ISort<T>
{

private:

public:
    BubbleSort() {}
    virtual void sort(T tab[],qint64 size) {
        T swap;
        for (int c = size - 1 ; c > 0; --c)
        {
            for (int d = 0 ; d < c; ++d)
            {
                if (tab[d] > tab[d+1]) /* For decreasing order use < */
                {
                    swap       = tab[d];
                    tab[d]   = tab[d+1];
                    tab[d+1] = swap;
                }
            }
        }
    }
};

#endif // BUBBLESORT_H
