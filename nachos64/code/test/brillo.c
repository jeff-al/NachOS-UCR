int main(){

int i,fd;
char buffer[ 100 ];
char filebuf[ 100 ];

Write("***** Iniciando Programa de Prueba #2 *****",44,1);
Write(" ",1,1);
Write("Escriba el nombre del archivo a leer",39,1);

i=Read(buffer,30,0);
if ( buffer[--i] == '\n' )
   buffer[i] = 0;

Write("Abriendo archivo", 17, 1);

fd = Open(buffer);

Write("Leyendo del archivo",22,1);

Read(filebuf,40,fd);

Write(filebuf,40,1);
Write("Cerrando archivo",17,1);

Close(fd);

//Halt();


}
