#include <iostream>
#include <string>

using namespace std;

class Student
{
private: 
    string name;
    string id;
    int numScores;
    int* scores;

public:
    Student(string n, string i, int* s, int num)
    {
        name = n;
        id = i;
        numScores = num;
        scores = new int[num];
        for (int j = 0; j < num; j++)
        {
            scores[j] = s[j];
        }
    }

    Student(const Student& other)
    {
        name = other.name;
        id = other.id;
        numScores = other.numScores;
        scores = new int[numScores];
        for (int j = 0; j < numScores; j++)
        {
            scores[j] = other.scores[j];
        }
    }

    ~Student()
    {
        delete[] scores;
    }

    void setScore(int index, int value) 
    {
        if (index >= 0 && index < numScores) 
        {
            scores[index] = value;
        }
    }

    void print() const 
    {
        cout << "姓名：" << name << "，学号：" << id << "，成绩：";
        for (int j = 0; j < numScores; j++) 
        {
            cout << scores[j] << " ";
        }
        cout << endl;
    }
};

int main()
{
    int s1[] = {85, 90, 78};
    Student stu1("张三", "2025001", s1, 3);

    Student stu2(stu1);

    stu1.setScore(0, 95);

    cout << "stu1：";
    stu1.print();
    cout << "stu2：";
    stu2.print();

    return 0;
}
