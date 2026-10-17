#include<iostream>
#include<iomanip>
#include"unitlib.h"
using namespace std;

int main()
{
    int n;
    double x;
    cout<<fixed<<setprecision(6);
    cout<<"UnitConverter v"<<UNIT_CONVERTER_VERSION<<"\n"
        <<"1. 摄氏度 → 华氏度\n2. 华氏度 → 摄氏度\n"
        <<"3. 米 → 英尺\n4. 英尺 → 米\n"
        <<"5. 磅 → 千克\n6. 千克 → 磅\n0. 退出\n请选择：";
    while(cin>>n)
    {
        if(n==0)break;
        if(n<1||n>6)
        {
            cout<<"请选择0～6：";
            continue;
        }
        cout<<"请输入数值：";
        if(!(cin>>x))break;
        cout<<"转换结果：";
        if(n==1)cout<<celsiusToFahrenheit(x)<<" 华氏度\n";
        else if(n==2)cout<<fahrenheitToCelsius(x)<<" 摄氏度\n";
        else if(n==3)cout<<metersToFeet(x)<<" 英尺\n";
        else if(n==4)cout<<feetToMeters(x)<<" 米\n";
        else if(n==5)cout<<poundsToKilograms(x)<<" 千克\n";
        else if(n==6)cout<<kilogramsToPounds(x)<<" 磅\n";
        cout<<"请选择：";
    }
    cout<<"程序结束。\n";
    return 0;
}
