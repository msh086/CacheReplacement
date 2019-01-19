#!/bin/python3
import numpy


class A_Period:
    def __init__(self, aleft, aright, a_num):
        self.left = aleft
        self.right = aright
        self.visit_number = a_num
        if self.left != self.right:
            self.avg = self.visit_number / (self.right - self.left)
        else:
            self.avg = 0


def work(filename):
    mylist = []

    with open(filename, 'r')as fin:
        while True:
            line = fin.readline()
            if not line: break
            a_time = float(line)
            a_period = A_Period(a_time, a_time, 1)
            mylist.append(a_period)
    it = 0
    while True:
        list_len = len(mylist)
        temp_list = []
        has_combine = False
        max_white_distants = mylist[1].left - mylist[1].right
        it += 1
        i = 0
        while i < (list_len - 1):
            temp_avg = (mylist[i].visit_number + mylist[i + 1].visit_number) / (
                    max(mylist[i].right, mylist[i + 1].right) - min(mylist[i].left, mylist[i + 1].left))
            # if temp_avg < mylist[i].avg and temp_avg < mylist[i + 1].avg:
            if temp_avg < min(mylist[i].avg, mylist[i + 1].avg) and mylist[i + 1].left - mylist[
                i].right < 10000 * max_white_distants:

                temp_list.append(mylist[i])
            else:
                # 合并
                has_combine = True
                temp_list.append(
                    A_Period(min(mylist[i].left, mylist[i + 1].left), max(mylist[i].right, mylist[i + 1].right),
                             mylist[i].visit_number + mylist[i + 1].visit_number))
                if len(temp_list) >= 2:
                    max_white_distants = max(max_white_distants, temp_list[-1].left - temp_list[-2].right)
                i += 1
            i += 1
        mylist = temp_list
        std = [x.avg for x in mylist]

        if len(std) <= 0:
            print("error")
            break
        else:
            print(numpy.std(std))
        if numpy.std(std) < 0.001:
            break
        if not has_combine:
            break
        with open("/home/find/d/b/huawei/b%d.txt" % it, 'w') as fout:
            for a in mylist:
                fout.write("%f, %f\t num:\t%d\tavg:\t%f\n" % (a.left, a.right, a.visit_number, a.avg))


work("/home/find/d/huawei/a.csv")
