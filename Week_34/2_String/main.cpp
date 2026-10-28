#include <iostream>
#include <sstream>
#include <vector>

using namespace std;

int main(void)
{
    string temp, line;
    vector<string> str;

    cout << "输入：" << endl;
    getline(cin, line);

    istringstream iss(line);

    while(iss >> temp)
    {
        str.push_back(temp);
    }

    cout << "单词个数：" << str.size() << endl << "逆序输出：";
    for(int i=str.size()-1 ; i>=0 ; i--)
    {
        cout << str[i] << " ";
    }

    return 0;
}