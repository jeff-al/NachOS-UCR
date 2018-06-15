int main(){
	int fd;
char * buf;// = new int[6];
	Create("archivo.txt");
	fd = Open("archivo.txt");
	Write("pruepa", 6, fd);
	Close(fd);
	int id = Exec("../test/brillo");	
	int id2 = Exec("../test/halt");
	Join(id2);
	//char* buf = new int[6];
	fd = Open("archivo.txt");
	Read(buf, 6, fd);
	Write(buf, 6, 1);

	Exit(0);
	fd = 1;
	fd++;
	return 0;
}
