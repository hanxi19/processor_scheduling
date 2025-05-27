//�ύ��һ������ʱ�䳬��100�������ҵ
#include <stdio.h>
#define N 10
int main(int argc,char *argv[])
{
	int i=0;
	for(i=0;i<argc;i++)
	{
		printf("%s\n",argv[i]);
	}

    int j=0;
    for(j=0;j<N;j++){//N=30,��������ʱ��Ϊ30s
	    sleep(1);
    }

	return 0;
}
