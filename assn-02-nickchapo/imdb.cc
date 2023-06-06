using namespace std;
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include "imdb.h"

const char *const imdb::kActorFileName = "actordata";
const char *const imdb::kMovieFileName = "moviedata";

imdb::imdb(const string& directory)
{
  const string actorFileName = directory + "/" + kActorFileName;
  const string movieFileName = directory + "/" + kMovieFileName;
  
  actorFile = acquireFileMap(actorFileName, actorInfo);
  movieFile = acquireFileMap(movieFileName, movieInfo);
}

bool imdb::good() const
{
  return !( (actorInfo.fd == -1) || 
	    (movieInfo.fd == -1) ); 
}

// you should be implementing these two methods right here... 
struct valuesActor{
  const char* value;
  const void* fileData;
};

struct valuesMovie{
  const char* value;
  const void* fileData;
  int year;
};


int cmprActr(const void * a, const void * b){
  
  valuesActor* actorData  = (valuesActor*) a;
  int* offset             = (int*) b;
  char* second            = (char*)actorData->fileData + *offset;
  //cout<<second<<endl;
  int result              = strcmp(actorData->value, second);

  return result;
}
int cmprFilm(const void * a, const void * b){
  valuesMovie* movieData  = (valuesMovie*) a;
  int* offset             = (int*) b;
  film first, second;
  first.title = movieData->value;
  first.year  = movieData->year;
  second.title= (char*)movieData->fileData + *offset;
  char yearChar;
  
  memcpy(&yearChar, (char*)movieData->fileData + *offset + second.title.length() + 1, sizeof(char));
  second.year = 1900 + (int)yearChar;

  if(first == second){return 0;}
  if(first < second){return -1;}
  return 1;
}

int getMoviesOffset(valuesActor actorData, int* ptr, short& movieNum){
  string player = actorData.value;
  int playerNameSize = player.length()+1;
  int movieNumOffset = playerNameSize + (playerNameSize%2) + *ptr;
  memcpy(&movieNum, (char*)actorData.fileData + movieNumOffset, sizeof(short));
  
  int moviesOffset = movieNumOffset + sizeof(short) ;
  moviesOffset += moviesOffset % 4;
  
  return moviesOffset;
}

bool imdb::getCredits(const string& player, vector<film>& films) const 
{ 
  int actorNum ;
  memcpy(&actorNum,(int*) actorFile,sizeof(int));
  valuesActor actorData;
  actorData.fileData  = actorFile;
  actorData.value     = player.c_str();

  int *ptr = (int*)bsearch( &actorData ,(int*)actorFile + 1, actorNum, sizeof(int), cmprActr);

  if(ptr == NULL) return false;

  short movieNum;
  int moviesOffset = getMoviesOffset(actorData, ptr, movieNum);

  for(int i=0; i<movieNum; i++){
    int filmOffset;
    memcpy(&filmOffset,(char*)actorFile + moviesOffset + (i * sizeof(int)), sizeof(int));
    char* filmTitle = (char*)movieFile + filmOffset;    
    char releaseYear;
    memcpy(&releaseYear, (char*)movieFile + filmOffset + strlen(filmTitle) + 1, sizeof(char));

    film filmtmp;
    filmtmp.title = filmTitle;
    filmtmp.year  = 1900 + (int) releaseYear;

    films.push_back(filmtmp);
  }

  return true; 
}

int getActorsOffset(valuesMovie movieData, int* ptr, short& actorNum){
  string movieName = movieData.value;
  int movieTitleLength    = movieName.length() + 1;
  int movieTitleWithYear  = movieTitleLength + sizeof(char);
  int actorNumOffset      = *ptr + movieTitleWithYear + movieTitleWithYear%2 ;

  memcpy(&actorNum, (char*)movieData.fileData + actorNumOffset, sizeof(short));
  int actorsOffset = actorNumOffset + sizeof(short) ;
  
  actorsOffset = actorsOffset + actorsOffset%4;

  return actorsOffset;
}

bool imdb::getCast(const film& movie, vector<string>& players) const { 
  int movieNum ;
  memcpy(&movieNum, (int*)movieFile, sizeof(int));
  valuesMovie movieData;
  movieData.fileData  = movieFile;
  movieData.value     = movie.title.c_str();
  movieData.year      = movie.year;

  int* ptr = (int*)bsearch(&movieData, (int*)movieFile + 1 ,movieNum , sizeof(int), cmprFilm);
  if(ptr ==  NULL) return false;

  short actorNum;
  int actorsOffset = getActorsOffset(movieData, ptr, actorNum);
  
  for (int i = 0; i < actorNum; i++){
    int actorOffset;
    memcpy(&actorOffset, (char*)movieFile + actorsOffset + (i * sizeof(int)), sizeof(int));
    char* actorName = (char*)actorFile + actorOffset;
    players.push_back(actorName);
  }
  return true; 
}

imdb::~imdb()
{
  releaseFileMap(actorInfo);
  releaseFileMap(movieInfo);
}

// ignore everything below... it's all UNIXy stuff in place to make a file look like
// an array of bytes in RAM.. 
const void *imdb::acquireFileMap(const string& fileName, struct fileInfo& info)
{
  struct stat stats;
  stat(fileName.c_str(), &stats);
  info.fileSize = stats.st_size;
  info.fd = open(fileName.c_str(), O_RDONLY);
  return info.fileMap = mmap(0, info.fileSize, PROT_READ, MAP_SHARED, info.fd, 0);
}

void imdb::releaseFileMap(struct fileInfo& info)
{
  if (info.fileMap != NULL) munmap((char *) info.fileMap, info.fileSize);
  if (info.fd != -1) close(info.fd);
}
