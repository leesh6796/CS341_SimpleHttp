#include <stdio.h>
#include <string.h>

int getFileSize(char *filename)
{
	FILE *fp = fopen(filename, "r");

	fseek(fp, 0, SEEK_END);
	int size = ftell(fp);

	fclose(fp);

	return size;
}


int main()
{
	int size = getFileSize("jelly.jpg");
	printf("jelly1 size = %d\n", getFileSize("jelly.jpg"));
	printf("jelly2 size = %d\n", getFileSize("jelly2.jpg"));

	FILE *fp1, *fp2;
	fp1 = fopen("jelly.jpg", "r");
	fp2 = fopen("jelly5.jpg", "r");

	char buf1[2];
	char buf2[2];

	int i;
	for(i=0; i<50; i++)
	{
		fread(buf1, 1, 1, fp1);
		fread(buf2, 1, 1, fp2);

		if(buf1[0] != buf2[0])
			printf("%d에서 다르다. jelly1=%d, jelly2=%d\n", i, buf1[0], buf2[0]);
		else
			printf("%d에서 같다. jelly1=%d, jelly2=%d\n", i, buf1[0], buf2[0]);

		memset(buf1, 0, 2);
		memset(buf2, 0, 2);
	}
	
	fclose(fp1);
	fclose(fp2);

	return 0;
}