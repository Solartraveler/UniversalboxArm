/*Coprocessor control for 128x128 and 320x240 LCD*/
#include <stdint.h>
const uint8_t menudata[] = {
5, 9, 3, 4, 1, 2, 28, 1, 0, 8, 0, 0, 5, 2, 2, 16, 
0, 8, 1, 0, 5, 2, 2, 32, 0, 0, 48, 2, 4, 2, 2, 48, 
0, 8, 2, 0, 4, 10, 102, 0, 42, 1, 3, 2, 81, 0, 60, 17, 
0, 3, 1, 98, 0, 60, 2, 100, 3, 63, 81, 0, 60, 17, 0, 3, 
0, 228, 0, 69, 2, 100, 3, 2, 100, 0, 60, 17, 0, 3, 0, 134, 
1, 79, 2, 100, 3, 63, 100, 0, 60, 17, 0, 3, 0, 188, 1, 82, 
2, 100, 8, 10, 10, 0, 108, 108, 0, 3, 4, 1, 2, 23, 11, 0, 
0, 86, 2, 4, 2, 23, 26, 0, 0, 100, 2, 4, 7, 15, 45, 0, 
20, 9, 0, 1, 3, 0, 0, 111, 2, 100, 0, 7, 60, 45, 0, 20, 
9, 0, 1, 3, 0, 0, 115, 2, 100, 16, 7, 15, 60, 0, 20, 9, 
0, 1, 3, 0, 0, 119, 2, 100, 32, 7, 60, 60, 0, 20, 9, 0, 
1, 3, 0, 0, 124, 2, 100, 48, 7, 15, 75, 0, 20, 9, 0, 1, 
3, 0, 0, 130, 2, 100, 64, 7, 60, 75, 0, 20, 9, 0, 1, 3, 
0, 0, 136, 2, 100, 80, 3, 40, 95, 0, 50, 17, 0, 3, 0, 255, 
255, 144, 2, 100, 8, 10, 10, 0, 108, 108, 0, 3, 4, 1, 2, 23, 
11, 0, 0, 149, 2, 4, 2, 23, 26, 0, 0, 163, 2, 4, 7, 15, 
45, 0, 20, 9, 0, 1, 5, 0, 0, 178, 2, 100, 1, 7, 15, 60, 
0, 20, 9, 0, 1, 5, 0, 0, 188, 2, 100, 17, 3, 40, 95, 0, 
50, 17, 0, 3, 0, 255, 255, 144, 2, 100, 9, 3, 4, 1, 2, 120, 
1, 0, 8, 0, 0, 5, 2, 120, 16, 0, 8, 1, 0, 4, 2, 80, 
46, 0, 0, 196, 2, 4, 2, 140, 46, 0, 8, 2, 0, 4, 3, 80, 
80, 0, 130, 17, 0, 3, 1, 98, 0, 207, 2, 100, 3, 80, 105, 0, 
130, 17, 0, 3, 0, 228, 0, 224, 2, 100, 3, 80, 130, 0, 130, 17, 
0, 3, 0, 134, 1, 237, 2, 100, 3, 80, 155, 0, 130, 18, 0, 3, 
0, 188, 1, 250, 2, 100, 8, 10, 10, 0, 108, 108, 0, 3, 4, 1, 
2, 14, 15, 0, 0, 3, 3, 4, 2, 14, 35, 0, 0, 16, 3, 4, 
3, 20, 85, 0, 30, 18, 0, 3, 0, 255, 255, 36, 3, 100, 3, 75, 
85, 0, 30, 18, 0, 3, 7, 255, 255, 39, 3, 100, 8, 10, 10, 0, 
108, 108, 0, 3, 4, 1, 2, 23, 11, 0, 0, 43, 3, 4, 2, 23, 
26, 0, 0, 51, 3, 4, 7, 15, 45, 0, 20, 9, 0, 1, 9, 0, 
0, 68, 3, 100, 2, 7, 60, 45, 0, 20, 9, 0, 1, 9, 0, 0, 
72, 3, 100, 18, 7, 15, 60, 0, 20, 9, 0, 1, 9, 0, 0, 77, 
3, 100, 34, 7, 60, 60, 0, 20, 9, 0, 1, 9, 0, 0, 82, 3, 
100, 50, 7, 15, 75, 0, 20, 9, 0, 1, 9, 0, 0, 88, 3, 100, 
66, 3, 40, 95, 0, 50, 17, 0, 3, 0, 255, 255, 144, 2, 100, 0, 
76, 97, 115, 116, 32, 115, 121, 110, 99, 58, 32, 0, 66, 71, 32, 67, 
111, 108, 111, 114, 0, 68, 105, 103, 32, 67, 111, 108, 111, 114, 0, 65, 
80, 0, 80, 87, 68, 0, 87, 97, 107, 101, 32, 117, 112, 32, 97, 108, 
97, 114, 109, 0, 111, 110, 32, 98, 97, 116, 116, 101, 114, 121, 0, 79, 
102, 102, 0, 49, 48, 115, 0, 49, 109, 105, 110, 0, 49, 48, 109, 105, 
110, 0, 49, 104, 111, 117, 114, 0, 49, 50, 104, 111, 117, 114, 115, 0, 
66, 97, 99, 107, 0, 77, 111, 100, 101, 32, 111, 110, 32, 85, 83, 66, 
45, 62, 0, 98, 97, 116, 116, 101, 114, 121, 32, 115, 119, 105, 116, 99, 
104, 0, 80, 111, 119, 101, 114, 32, 111, 102, 102, 0, 83, 116, 97, 121, 
32, 111, 110, 0, 76, 97, 115, 116, 32, 115, 121, 110, 99, 58, 0, 66, 
97, 99, 107, 103, 114, 111, 117, 110, 100, 32, 99, 111, 108, 111, 114, 0, 
68, 105, 103, 105, 116, 115, 32, 99, 111, 108, 111, 114, 0, 65, 99, 99, 
101, 115, 115, 32, 112, 111, 105, 110, 116, 0, 80, 97, 115, 115, 119, 111, 
114, 100, 0, 82, 101, 97, 108, 108, 121, 32, 114, 101, 115, 101, 116, 0, 
98, 97, 116, 116, 101, 114, 121, 32, 115, 116, 97, 116, 105, 115, 116, 105, 
99, 115, 63, 0, 78, 111, 0, 89, 101, 115, 0, 77, 97, 120, 105, 109, 
117, 109, 0, 99, 104, 97, 114, 103, 105, 110, 103, 32, 99, 117, 114, 114, 
101, 110, 116, 0, 48, 109, 65, 0, 52, 48, 109, 65, 0, 55, 48, 109, 
65, 0, 49, 48, 48, 109, 65, 0, 49, 53, 48, 109, 65, 0};
