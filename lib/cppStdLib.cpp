#include <iostream>
#include <string>
void print(char ch) {
    std::cout << ch;
}
void print(const char* str) {
    std::cout << str;
}
void print(unsigned int i) {
    std::cout << i;
}
void print(unsigned long long int i) {
    std::cout << i;
}
void print(int i) {
    std::cout << i;
}
void print(long long int i) {
    std::cout << i;
}
void print(float f) {
    std::cout << f;
}
void print(double f) {
    std::cout << f;
}

void println(char ch) {
    std::cout << ch << '\n';
}
void println(const char* str) {
    std::cout << str << '\n';
}
void println(unsigned int i) {
    std::cout << i << '\n';
}
void println(unsigned long long int i) {
    std::cout << i << '\n';
}
void println(int i) {
    std::cout << i << '\n';
}
void println(long long int i) {
    std::cout << i << '\n';
}
void println(float f) {
    std::cout << f << '\n';
}
void println(double f) {
    std::cout << f << '\n';
}
void println() {
    std::cout << '\n';
}

int strToInt(const char* str) {
    return std::stoi(str);
}
unsigned int strToUInt(const char* str) {
    return std::stoi(str);
}
/*
int main() {
    strToUInt("0");
    return 0;
}
//*/