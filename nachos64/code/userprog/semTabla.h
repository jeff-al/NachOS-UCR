#include "bitmap.h"
#include "synch.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define maxSem 128

class SemTabla {
  public:
    SemTabla();       // Initialize
    ~SemTabla();      // De-allocate

    int Create(Semaphore *val); // Register the file handle
    int Close( int id );      // Unregister the file handle
    bool isCreate( int id );
    Semaphore getSemaphore( int id );
    void addThread();		// If a user thread is using this table, add it
    void delThread();		// If a user thread is using this table, delete it

    void Print();               // Print contents

  private:
    Semaphore  semaforosAbiertos[maxSem];		// A vector with user opened files
    BitMap * semaforosMap;	// A bitmap to control our vector
    int usage;			// How many threads are using this table

};
