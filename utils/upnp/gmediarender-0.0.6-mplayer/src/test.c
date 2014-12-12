/*************************************************************************
    > File Name: test.c
    > Author: yuehb
    > Mail: yuehb@bellnett.com 
    > Created Time: Tue 02 Sep 2014 03:40:02 PM CST
 ************************************************************************/

#include <stdio.h>

int main()
{

	system("mplayer -slave -input file=/tmp/mpg123_pipe http://172.16.0.2/2.mp3 &");
}
