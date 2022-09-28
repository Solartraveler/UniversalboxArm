/*Coprocessor control for 128x128 and 320x240 LCD*/
#include <stdint.h>
const uint8_t menudata[] = {
5, 9, 3, 4, 1, 2, 2, 1, 0, 0, 4, 6, 4, 10, 102, 0, 
223, 2, 2, 60, 1, 0, 8, 0, 0, 4, 2, 2, 16, 0, 0, 15, 
6, 4, 2, 60, 16, 0, 8, 1, 0, 4, 2, 2, 31, 0, 0, 20, 
6, 4, 2, 60, 31, 0, 8, 2, 0, 4, 2, 2, 46, 0, 0, 25, 
6, 4, 2, 60, 46, 0, 8, 3, 0, 4, 2, 2, 61, 0, 0, 33, 
6, 4, 2, 60, 61, 0, 8, 4, 0, 4, 3, 2, 81, 0, 73, 17, 
0, 3, 1, 0, 0, 42, 6, 100, 3, 77, 81, 0, 47, 17, 0, 3, 
0, 181, 5, 53, 6, 100, 3, 2, 100, 0, 60, 17, 0, 3, 0, 23, 
2, 57, 6, 100, 3, 63, 100, 0, 60, 17, 0, 3, 0, 153, 2, 67, 
6, 100, 10, 2, 3, 151, 0, 9, 3, 4, 1, 2, 2, 1, 0, 0, 
76, 6, 4, 10, 2, 0, 32, 1, 2, 2, 16, 0, 0, 85, 6, 4, 
2, 60, 16, 0, 8, 5, 0, 4, 2, 2, 31, 0, 0, 92, 6, 4, 
2, 60, 31, 0, 8, 6, 0, 4, 2, 2, 46, 0, 0, 101, 6, 4, 
2, 60, 46, 0, 8, 7, 0, 4, 2, 2, 61, 0, 0, 110, 6, 4, 
2, 60, 61, 0, 8, 8, 0, 4, 2, 2, 76, 0, 0, 116, 6, 4, 
2, 60, 76, 0, 8, 9, 0, 4, 2, 2, 91, 0, 0, 124, 6, 4, 
2, 60, 91, 0, 8, 10, 0, 4, 2, 2, 106, 0, 0, 133, 6, 4, 
2, 60, 106, 0, 8, 11, 0, 4, 2, 60, 1, 0, 8, 12, 0, 4, 
9, 3, 4, 1, 2, 40, 1, 0, 0, 76, 6, 4, 10, 2, 0, 153, 
1, 2, 2, 16, 0, 0, 138, 6, 4, 2, 80, 16, 0, 8, 13, 0, 
4, 2, 2, 31, 0, 0, 153, 6, 4, 2, 80, 31, 0, 8, 14, 0, 
4, 2, 2, 46, 0, 0, 161, 6, 4, 2, 80, 46, 0, 8, 15, 0, 
4, 3, 2, 63, 0, 75, 18, 0, 3, 0, 214, 4, 173, 6, 100, 3, 
79, 63, 0, 47, 18, 0, 3, 0, 66, 5, 185, 6, 100, 3, 2, 82, 
0, 124, 18, 0, 3, 5, 0, 0, 193, 6, 100, 3, 2, 101, 0, 124, 
18, 0, 3, 0, 12, 5, 211, 6, 100, 9, 0, 0, 0, 10, 2, 7, 
1, 0, 2, 1, 0, 0, 0, 76, 6, 4, 2, 50, 0, 0, 8, 16, 
0, 4, 1, 3, 16, 0, 122, 1, 0, 0, 0, 0, 0, 0, 1, 3, 
97, 0, 122, 1, 0, 0, 0, 0, 0, 0, 1, 3, 17, 0, 1, 80, 
0, 0, 0, 0, 0, 0, 1, 124, 17, 0, 1, 80, 0, 0, 0, 0, 
0, 0, 4, 4, 17, 0, 120, 80, 0, 76, 0, 0, 0, 0, 0, 2, 
13, 100, 0, 8, 17, 0, 4, 2, 13, 114, 0, 8, 18, 0, 4, 1, 
3, 100, 0, 8, 12, 0, 0, 0, 0, 0, 4, 1, 3, 114, 0, 8, 
12, 0, 0, 0, 0, 0, 1, 8, 10, 10, 0, 108, 108, 0, 3, 4, 
1, 2, 23, 11, 0, 0, 228, 6, 4, 2, 23, 26, 0, 0, 242, 6, 
4, 7, 15, 45, 0, 20, 9, 0, 1, 9, 0, 0, 253, 6, 100, 0, 
7, 60, 45, 0, 20, 9, 0, 1, 9, 0, 0, 1, 7, 100, 16, 7, 
15, 60, 0, 20, 9, 0, 1, 9, 0, 0, 5, 7, 100, 32, 7, 60, 
60, 0, 20, 9, 0, 1, 9, 0, 0, 10, 7, 100, 48, 7, 15, 75, 
0, 20, 9, 0, 1, 9, 0, 0, 16, 7, 100, 64, 7, 60, 75, 0, 
20, 9, 0, 1, 9, 0, 0, 22, 7, 100, 80, 3, 40, 95, 0, 50, 
17, 0, 3, 0, 255, 255, 30, 7, 100, 8, 10, 10, 0, 108, 108, 0, 
3, 4, 1, 2, 23, 11, 0, 0, 35, 7, 4, 2, 23, 26, 0, 0, 
49, 7, 4, 7, 15, 45, 0, 20, 9, 0, 1, 11, 0, 0, 64, 7, 
100, 1, 7, 15, 60, 0, 20, 9, 0, 1, 11, 0, 0, 74, 7, 100, 
17, 3, 40, 95, 0, 50, 17, 0, 3, 0, 255, 255, 30, 7, 100, 9, 
3, 4, 1, 2, 80, 1, 0, 0, 4, 6, 4, 2, 140, 1, 0, 8, 
0, 0, 4, 2, 80, 16, 0, 0, 15, 6, 4, 2, 140, 16, 0, 8, 
1, 0, 4, 2, 80, 31, 0, 0, 20, 6, 4, 2, 140, 31, 0, 8, 
2, 0, 4, 2, 80, 46, 0, 0, 25, 6, 4, 2, 140, 46, 0, 8, 
3, 0, 4, 2, 80, 61, 0, 0, 33, 6, 4, 2, 140, 61, 0, 8, 
4, 0, 4, 3, 80, 80, 0, 130, 17, 0, 3, 1, 0, 0, 42, 6, 
100, 3, 80, 105, 0, 130, 17, 0, 3, 0, 23, 2, 57, 6, 100, 3, 
80, 130, 0, 130, 17, 0, 3, 0, 153, 2, 82, 7, 100, 3, 80, 155, 
0, 130, 18, 0, 3, 0, 181, 5, 102, 7, 100, 10, 2, 3, 112, 3, 
9, 3, 4, 1, 10, 2, 7, 223, 2, 2, 80, 1, 0, 0, 76, 6, 
4, 2, 2, 16, 0, 0, 110, 6, 4, 2, 110, 16, 0, 8, 8, 0, 
4, 2, 2, 31, 0, 0, 115, 7, 4, 2, 110, 31, 0, 8, 9, 0, 
4, 2, 2, 46, 0, 0, 101, 6, 4, 2, 110, 46, 0, 8, 7, 0, 
4, 2, 2, 61, 0, 0, 122, 7, 4, 2, 110, 61, 0, 8, 5, 0, 
4, 2, 2, 76, 0, 0, 135, 7, 4, 2, 110, 76, 0, 8, 6, 0, 
4, 2, 2, 91, 0, 0, 133, 6, 4, 2, 110, 91, 0, 8, 11, 0, 
4, 2, 2, 106, 0, 0, 124, 6, 4, 2, 110, 106, 0, 8, 10, 0, 
4, 2, 2, 121, 0, 0, 145, 7, 4, 2, 110, 121, 0, 8, 13, 0, 
4, 2, 2, 136, 0, 0, 159, 7, 4, 2, 110, 136, 0, 8, 14, 0, 
4, 2, 2, 151, 0, 0, 174, 7, 4, 2, 110, 151, 0, 8, 15, 0, 
4, 2, 2, 166, 0, 0, 192, 7, 4, 2, 110, 166, 0, 8, 12, 0, 
4, 2, 200, 137, 0, 8, 17, 0, 4, 2, 200, 151, 0, 8, 18, 0, 
4, 2, 200, 166, 0, 8, 16, 0, 4, 3, 50, 185, 0, 100, 18, 0, 
3, 0, 214, 4, 173, 6, 100, 3, 170, 185, 0, 100, 18, 0, 3, 0, 
66, 5, 198, 7, 100, 3, 50, 210, 0, 100, 18, 0, 3, 5, 0, 0, 
193, 6, 100, 3, 170, 210, 0, 100, 18, 0, 3, 0, 12, 5, 211, 6, 
100, 1, 190, 47, 0, 122, 1, 0, 0, 0, 0, 0, 0, 1, 190, 128, 
0, 122, 1, 0, 0, 0, 0, 0, 0, 1, 190, 48, 0, 1, 80, 0, 
0, 0, 0, 0, 0, 1, 55, 48, 16, 1, 80, 0, 0, 0, 0, 0, 
0, 4, 191, 48, 0, 120, 80, 0, 76, 0, 0, 0, 0, 0, 1, 190, 
137, 0, 8, 12, 0, 0, 0, 0, 0, 4, 1, 190, 151, 0, 8, 12, 
0, 0, 0, 0, 0, 1, 8, 10, 10, 0, 108, 108, 0, 3, 4, 1, 
2, 23, 20, 0, 0, 214, 7, 4, 2, 23, 35, 0, 0, 227, 7, 4, 
3, 20, 85, 0, 30, 18, 0, 3, 0, 255, 255, 242, 7, 100, 3, 75, 
85, 0, 30, 18, 0, 3, 13, 255, 255, 245, 7, 100, 8, 10, 10, 0, 
108, 108, 0, 3, 4, 1, 2, 14, 15, 0, 0, 214, 7, 4, 2, 14, 
35, 0, 0, 249, 7, 4, 3, 20, 85, 0, 30, 18, 0, 3, 0, 255, 
255, 242, 7, 100, 3, 75, 85, 0, 30, 18, 0, 3, 15, 255, 255, 245, 
7, 100, 8, 10, 10, 0, 108, 108, 0, 3, 4, 1, 2, 23, 11, 0, 
0, 13, 8, 4, 2, 23, 26, 0, 0, 21, 8, 4, 7, 15, 45, 0, 
20, 9, 0, 1, 17, 0, 0, 38, 8, 100, 2, 7, 60, 45, 0, 20, 
9, 0, 1, 17, 0, 0, 42, 8, 100, 18, 7, 15, 60, 0, 20, 9, 
0, 1, 17, 0, 0, 47, 8, 100, 34, 7, 60, 60, 0, 20, 9, 0, 
1, 17, 0, 0, 52, 8, 100, 50, 7, 15, 75, 0, 20, 9, 0, 1, 
17, 0, 0, 58, 8, 100, 66, 3, 40, 95, 0, 50, 17, 0, 3, 0, 
255, 255, 30, 7, 100, 8, 10, 10, 0, 108, 108, 0, 3, 4, 1, 2, 
23, 11, 0, 0, 64, 8, 4, 2, 23, 26, 0, 0, 77, 8, 4, 2, 
23, 41, 0, 0, 87, 8, 4, 7, 40, 60, 0, 20, 9, 0, 1, 19, 
0, 0, 253, 6, 100, 3, 7, 40, 80, 0, 20, 9, 0, 1, 19, 0, 
0, 95, 8, 100, 19, 3, 40, 95, 0, 50, 17, 0, 3, 0, 255, 255, 
30, 7, 100, 0, 70, 105, 114, 109, 119, 97, 114, 101, 58, 32, 0, 86, 
99, 99, 58, 0, 67, 80, 85, 58, 0, 85, 112, 116, 105, 109, 101, 58, 
0, 79, 112, 116, 105, 109, 101, 58, 32, 0, 80, 111, 119, 101, 114, 32, 
100, 111, 119, 110, 0, 76, 69, 68, 0, 83, 101, 116, 32, 97, 108, 97, 
114, 109, 0, 83, 101, 116, 32, 109, 111, 100, 101, 0, 66, 97, 116, 116, 
101, 114, 121, 58, 0, 84, 101, 109, 112, 58, 32, 0, 86, 111, 108, 116, 
97, 103, 101, 58, 0, 67, 117, 114, 114, 101, 110, 116, 58, 0, 77, 111, 
100, 101, 58, 0, 69, 114, 114, 111, 114, 58, 32, 0, 67, 104, 97, 114, 
103, 101, 100, 58, 0, 80, 87, 77, 58, 0, 84, 111, 116, 97, 108, 32, 
99, 104, 97, 114, 103, 101, 58, 32, 0, 67, 121, 99, 108, 101, 115, 58, 
0, 80, 114, 101, 32, 99, 121, 99, 108, 101, 115, 58, 0, 78, 101, 119, 
32, 98, 97, 116, 116, 101, 114, 121, 0, 67, 117, 114, 114, 101, 110, 116, 
0, 70, 111, 114, 99, 101, 32, 102, 117, 108, 108, 32, 99, 104, 97, 114, 
103, 101, 0, 82, 101, 115, 101, 116, 32, 115, 116, 97, 116, 105, 115, 116, 
105, 99, 115, 0, 87, 97, 107, 101, 32, 117, 112, 32, 97, 108, 97, 114, 
109, 0, 111, 110, 32, 98, 97, 116, 116, 101, 114, 121, 0, 79, 102, 102, 
0, 49, 48, 115, 0, 49, 109, 105, 110, 0, 49, 48, 109, 105, 110, 0, 
49, 104, 111, 117, 114, 0, 49, 50, 104, 111, 117, 114, 115, 0, 66, 97, 
99, 107, 0, 77, 111, 100, 101, 32, 111, 110, 32, 85, 83, 66, 45, 62, 
0, 98, 97, 116, 116, 101, 114, 121, 32, 115, 119, 105, 116, 99, 104, 0, 
80, 111, 119, 101, 114, 32, 111, 102, 102, 0, 83, 116, 97, 121, 32, 111, 
110, 0, 83, 101, 116, 32, 112, 111, 119, 101, 114, 32, 100, 111, 119, 110, 
32, 109, 111, 100, 101, 0, 83, 101, 116, 32, 76, 69, 68, 32, 109, 111, 
100, 101, 0, 69, 114, 114, 111, 114, 58, 0, 84, 101, 109, 112, 101, 114, 
97, 116, 117, 114, 101, 58, 0, 86, 111, 108, 116, 97, 103, 101, 58, 32, 
0, 84, 111, 116, 97, 108, 32, 99, 104, 97, 114, 103, 101, 100, 0, 67, 
104, 97, 114, 103, 101, 32, 99, 121, 99, 108, 101, 115, 58, 0, 80, 114, 
101, 99, 104, 97, 114, 103, 101, 32, 99, 121, 99, 108, 101, 115, 58, 0, 
84, 105, 109, 101, 58, 0, 83, 101, 116, 32, 109, 97, 120, 32, 99, 117, 
114, 114, 101, 110, 116, 0, 82, 101, 97, 108, 108, 121, 32, 114, 101, 115, 
101, 116, 0, 98, 97, 116, 116, 101, 114, 121, 32, 101, 114, 114, 111, 114, 
63, 0, 78, 111, 0, 89, 101, 115, 0, 98, 97, 116, 116, 101, 114, 121, 
32, 115, 116, 97, 116, 105, 115, 116, 105, 99, 115, 63, 0, 77, 97, 120, 
105, 109, 117, 109, 0, 99, 104, 97, 114, 103, 105, 110, 103, 32, 99, 117, 
114, 114, 101, 110, 116, 0, 48, 109, 65, 0, 52, 48, 109, 65, 0, 55, 
48, 109, 65, 0, 49, 48, 48, 109, 65, 0, 49, 53, 48, 109, 65, 0, 
76, 69, 68, 32, 99, 111, 110, 102, 105, 114, 109, 115, 0, 101, 118, 101, 
114, 121, 32, 83, 80, 73, 0, 99, 111, 109, 109, 97, 110, 100, 0, 79, 
110, 0};