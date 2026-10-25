#include <iostream>
#include <vector>
#include <algorithm>
#include <set>

using namespace std;

int main(void)
{
    int size;
    vector<int> num;

    cout << "input size" << endl;
    cin >> size;
    cout << "input num" << endl;
    for(int i=0; i<size; i++)
    {
        int temp;
        cin >> temp;
        num.push_back(temp);
    }

    sort(num.begin(), num.end());
    cout << "排序输出" << endl;
    for(int i=0; i<size; i++)
    {
        cout << num[i] << " ";
    }

    set<int> s(num.begin(), num.end());
    cout << "\n去重输出" << endl;
    for (const int& num : s) {
        cout << num << " ";
    }

    return 0;
}