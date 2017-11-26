
//Adds short to buf in network byte order starting from index startPoint
void addShort(unsigned char buf[], int startPoint, unsigned short numToAdd) {
	buf[startPoint] = numToAdd >> 8;
	buf[startPoint + 1] = numToAdd;
}

//Reads short from buf in host byte order starting from index startPoint
unsigned short readShort(unsigned char buf[], int startPoint) {
	unsigned short numIWant = buf[startPoint] << 8 | buf[startPoint + 1];
	return numIWant;
}

//Adds long to buf in network byte order starting from index startPoint
void addLong(unsigned char buf[], int startPoint, unsigned short numToAdd) {
	buf[startPoint] = numToAdd >> 24;
	buf[startPoint + 1] = numToAdd >> 16;
	buf[startPoint + 2] = numToAdd >> 8;
	buf[startPoint + 3] = numToAdd;
}

//Reads long from buf in host byte order starting from index startPoint
unsigned long readLong(unsigned char buf[], int startPoint) {
	unsigned long numIWant = buf[startPoint] << 24 | buf[startPoint + 1] << 16 | buf[startPoint + 2] << 8 | buf[startPoint + 3];
}

//had to have this to compile for some reason
int main() {
	return 0;
}