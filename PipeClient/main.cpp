/* from http://ist.marshall.edu/ist480acp/code/pipec.cpp */

#include <windows.h>
#include <iostream>

//---------------------------------------------------------------------------

using namespace std;

int main()
{
   HANDLE npipe;

   if( ! WaitNamedPipe(L"\\\\.\\pipe\\WinBtrfsService", 25000) ){
      cerr<<"Error: cannot wait for the named pipe. Error code: "
          <<GetLastError<<endl;
      return 1;
   }

   npipe = CreateFile(L"\\\\.\\pipe\\WinBtrfsService",
                       GENERIC_READ | GENERIC_WRITE,
                       0, NULL, OPEN_EXISTING, 0, NULL);
   if( npipe == INVALID_HANDLE_VALUE ){
      cerr<<"Error: cannot open named pipe\n";
      return 1;
   }
   #ifdef TRACE_ON
   cerr<<"Named pipe opened successfully"<<endl;
   cerr<<"Connecting to the server ...\n";
   #endif

   char  buf[1024];
   DWORD bread;
   for(int i=0;i<15;i++){
      sprintf(buf, "Message #%i from the client.", i+1);
      if( !WriteFile(npipe, (void*)buf, strlen(buf)+1, &bread, NULL) ){
         cerr<<"Error writing the named pipe\n";
      }
      if( ReadFile(npipe, (void*)buf, 1023, &bread, NULL) ){
         buf[bread] = 0;
         cout<<"Received: '"<<buf<<"'"<<endl;
      }
      Sleep(750);
   }

   return 0;
}
//---------------------------------------------------------------------------

