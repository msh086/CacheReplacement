#include<iostream>
#include<fstream>
#include<cstring>
using namespace std;

typedef unsigned long long ull;

/**
 * Read a 32 bit fixed length hex number from string
*/
uint readHex(const char* src){
    uint ans = 0;
    for(int i = 0; i < 8; i++){
        if(src[i] <= '9')
            ans = (ans << 4) + src[i] - '0';
        else
            ans = (ans << 4) + src[i] - 'a' + 10;
    }
    return ans;
}

/**
 * Read a 64 bit hex number from string, terminated by ' '
*/
ull read64(const char* src){
    ull ans = 0;
    for(int i = 0; src[i] != ' '; i++){
        if(src[i] <= '9')
            ans = (ans << 4) + src[i] - '0';
        else
            ans = (ans << 4) + src[i] - 'a' + 10;
    }
    return ans;
}

/**
 * Get the actual bits used by num
*/
int bit_length(ull num){
    int ans = 0;
    while(num){
        num >>= 1;
        ans++;
    }
    return ans;
}

// Get the offset of nth field in src, fields seperated by ' ', n starts from 0
uint seek_field(const char* src, int n){
    int i = 0;
    while(src[i] && n){
        if(src[i] == ' ')
            n--;
        i++;
    }
    return i;
}

int main(int argc, char** argv){
    if(argc != 2){
        printf("Usage: ./stat [filename]\n");
        return 0;
    }
    ifstream fin;
    char buffer[100] = "";
    fin.open(argv[1]);
    if(fin.fail()){
        printf("Opening file failed\n");
        fin.close();
        return 0;
    }
    // int higherBits = 0;
    // printf("Enter higher bits count: ");
    // scanf("%d", &higherBits);
    // if(higherBits < 1 || higherBits > 10){
    //     printf("higher bit should lie in [1, 10]\n");
    //     return 0;
    // }
    ull header[4] = {0};
    ull cpu = 0, gpu = 0;
    ull line = 0;
    // ull* counter = new ull[1 << higherBits];
    // memset(counter, 0, (1 << higherBits) * sizeof(ull));
    while(!fin.eof()){
        fin.getline(buffer, 100);
        if(!buffer[0])
            break;
        line++;
        ull addr = read64(buffer + seek_field(buffer, 1));
        header[addr >> 32]++;
        // if(buffer[seek_field(buffer, 4) + 1] <= '9')
        //     cpu++;
        // else
        //     gpu++;
        // counter[(readHex(buffer + seek_field(buffer, 1))) >> (32 - higherBits)]++;
        memset(buffer, 0, 100);
    }
    fin.close();
    // for(int i = 0; i < (1 << higherBits); i++)
        // printf("%#x %llu\n", i, counter[i]);
    // delete[] counter;
    for(int i = 0; i < 4; i++)
        printf("MSB:%x count:%llu percentage:%f\n", i, header[i], (double)header[i] / line);
    printf("cpu: %llu, gpu: %llu\n", cpu, gpu);
    return 0;
}