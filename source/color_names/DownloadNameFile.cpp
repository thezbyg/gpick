#include "DownloadNameFile.h"

#include <sstream>
#include <fstream>

using namespace std;

#ifdef DOWNLOAD_RESENE_COLOR_LIST

#include <curl/curl.h>

static size_t data_write_function(void *ptr, size_t size, size_t nmemb, stringstream *data)
{
	(*data).write(reinterpret_cast<char*>(ptr), size * nmemb);
	return size * nmemb;
}

int download_name_file(const char *destination_filename)
{
  CURL *curl;
  CURLcode res;

  curl = curl_easy_init();
  if (curl) {
		stringstream data;

    curl_easy_setopt(curl, CURLOPT_URL, "http://gpick.googlecode.com/hg/share/gpick/colors.txt");
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, data_write_function);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);

    res = curl_easy_perform(curl);
    if (res != CURLE_OK){
			curl_easy_cleanup(curl);
			return -1;
		}

    ofstream fout(destination_filename);
		if (fout.is_open()){
			string data_str = data.str();
			fout.write(data_str.c_str(), data_str.length());
      fout.close();
		}

    curl_easy_cleanup(curl);
  }else return -1;
  return 0;
}

#else

int download_name_file(const char *destination_filename)
{
	return -1;
}

#endif
