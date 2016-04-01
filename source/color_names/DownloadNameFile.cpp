/*
 * Copyright (c) 2009-2016, Albertas Vyšniauskas
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of the software author nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

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
		curl_easy_setopt(curl, CURLOPT_URL, "http://static.gpick.org/palette/colors.txt");
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
