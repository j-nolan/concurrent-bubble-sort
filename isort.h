#ifndef ISORT_H
#define ISORT_H

template<typename T>
class ISort
{
public:
    virtual void sort(T tab[],qint64 size) = 0;
};

#endif // ISORT_H
