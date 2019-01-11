#!/bin/python3
import numpy


class A_Period:
    def __init__(self, aleft, aright, a_num):
        self.left = aleft
        self.right = aright
        self.visit_number = a_num
        # 平均每ns有多少次访问
        self.avg = self.visit_number / (self.right - self.left + 1)


def work(filename):
    mylist = []

    with open(filename, 'r')as fin:
        while True:
            line = fin.readline()
            if not line: break
            a_time = float(line)
            a_period = A_Period(a_time, a_time, 1)
            mylist.append(a_period)

    while True:
        list_len = len(mylist)
        temp_list = []
        has_combine = False
        for i in range(list_len - 1):
            temp_avg = (mylist[i].visit_number + mylist[i + 1].visit_number) / (
                    max(mylist[i].right, mylist[i + 1].right) - min(mylist[i].left, mylist[i + 1].left) + 1)
            if temp_avg < mylist[i].avg and temp_avg < mylist[i + 1].avg:
                temp_list.append(mylist[i])
            else:
                # 合并
                has_combine = True
                temp_list.append(
                    A_Period(min(mylist[i].left, mylist[i + 1].left), max(mylist[i].right, mylist[i + 1].right),
                             mylist[i].visit_number + mylist[i + 1].visit_number))
                i += 1
        mylist = temp_list
        std = [x.avg for x in mylist]
        print(numpy.std(std))
        if not has_combine:
            break


work("/home/find/d/huawei/acc_page0d302.csv")
