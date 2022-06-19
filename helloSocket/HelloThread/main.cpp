#include<iostream>
#include<windows.h>
#include<thread>
#include<mutex> // 锁
#include<atomic> // 原子
#include "CELLTimestamp.hpp"
using namespace std;
// 原子操作 原子 分子
mutex m;
const int cCount = 4;
atomic_int num = 0;
void workFun(int index) {
	for (int i = 0; i < 2000000; i++) {
		//自解锁
		//lock_guard<mutex> lg(m);
		//m.lock();
		//cout << index << "HELLO ,other THREAD" << i << endl;
		num++;
		//m.unlock();
		
	}
}// 抢占式
int main() {
	CELLTimestamp tTime;
	thread t[cCount];
	for (int i = 0; i < cCount; i++) {
		t[i] = thread(workFun, i);
	}
	for (int i = 0; i < cCount; i++) {
		t[i].join();
		//t[i].detach();
	}
	cout<<"time="<<tTime.getElapsedSecond()<<",num="<< num<<endl;
	tTime.update();
	//t.detach();
	//t.join();
	cout << "num:" << num << endl;
	int cnum = 0;
	for (int i = 0; i < 8000000; i++) {
		cnum++;

	}
	cout << "time=" << tTime.getElapsedSecond() << ",cnum:" << cnum << endl;
	//	cout <<"HELLO ,MAIN THREAD"<< endl;
	//Sleep(1000000);
	system("pause");
	return 1;
}