#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <curl/curl.h>
#include <unistd.h>
#include <string.h>

void main() {
    char title[25];
    long puser[17], pnice[17], psys[17], pidle[17], piowait[17], phi[17], psi[17], psteal[17], pguest[17], pgnice[17];
    long user[17], nice[17], sys[17], idle[17], iowait[17], hi[17], si[17], steal[17], guest[17], gnice[17];
    double usage[3][17];
    int r = 0;
    int cores = 0;
    int iterations = 3;
    int delay = 1;
    CURL *curl;
    CURLcode res;

    for(int itr=0; itr<iterations; itr++)
    {
	int c = 0;
	FILE *f = fopen("/proc/stat", "r");
	if (f == NULL)
	{
            return;
	}

        for (c=0; c<=17; c++)
        {
    	    r = fscanf(f, "%s %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld", title, &puser[c], &pnice[c], &psys[c], &pidle[c], &piowait[c], &phi[c], &psi[c], &psteal[c], &pguest[c], &pgnice[c]);
	    if (r == EOF || title[0] != 'c' || title[1] != 'p' || title[2] != 'u')
	    {
	       c--;
	       break;
	    }
        }
	fclose(f);

	sleep(delay);

	f = fopen("/proc/stat", "r");
        if (f == NULL)
        {
            return;
        }
        for (int itr2=0; itr2<=c; itr2++)
        {
            fscanf(f, "%s %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld", title, &user[itr2], &nice[itr2], &sys[itr2], &idle[itr2], &iowait[itr2], &hi[itr2], &si[itr2], &steal[itr2], &guest[itr2], &gnice[itr2]);
	    user[itr2] = user[itr2] - puser[itr2];
	    nice[itr2] = nice[itr2] - pnice[itr2];
	    sys[itr2] = sys[itr2] - psys[itr2];
	    idle[itr2] = idle[itr2] - pidle[itr2];
	    iowait[itr2] = iowait[itr2] - piowait[itr2];
	    hi[itr2] = hi[itr2] - phi[itr2];
	    si[itr2] = si[itr2] - psi[itr2];
	    steal[itr2] = steal[itr2] - psteal[itr2];
	    guest[itr2] = guest[itr2] - pguest[itr2];
	    gnice[itr2] = gnice[itr2] - pgnice[itr2];
        }
	fclose(f);

	for (int cr=0; cr<=c; cr++)
        {
	    long total = user[cr] + nice[cr] + sys[cr] + idle[cr] + iowait[cr] + hi[cr] + si[cr] + steal[cr];
	    long idl = idle[cr] + iowait[cr];
	    usage[itr][cr] = ((double)(total - idl)/total) * 100;
	}
	cores = c;
    }

    for (int i=1; i<=cores; i++)
    {
	//printf("%f\n", usage[0][i]);
	for (int j=1; j<iterations; j++)
	{
	    //printf("%f\n", usage[j][i]);
	    usage[0][i] += usage[j][i];
	}
	//printf("%f %d\n", usage[0][i], iterations);
	usage[0][i] = usage[0][i]/iterations;
    }

    char data[500];
    int length = 0;
    for (int i=1; i<=cores; i++)
    {
	length += snprintf(data + length, sizeof(data), "cpu_usage,host=RPi3b,core=%d value=%f\n", i-1, usage[0][i]);
	if (length < 0)
	{
	    printf("Error while dumping data to char array");
	    return;
	}
    }
//    printf("%s\n", data);

    curl = curl_easy_init();
    if(curl)
    {
    	curl_easy_setopt(curl, CURLOPT_URL, "http://192.168.0.137:8086/write?db=perfdb");
    	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
    	res = curl_easy_perform(curl);

    	if(res != CURLE_OK)
      	    fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
	else
	    fprintf(stderr, "curl_easy_perform() succeeded");
    	curl_easy_cleanup(curl);
    }

    long totalMem = 0;
    long usedMem = 0;
    long swaptotal = 0;
    long swapfree = 0;
    double swappct = 0.0;
    FILE *f = fopen("/proc/meminfo", "r");
    if (f != NULL)
    {
	long tmp = 0;
	r = fscanf(f, "%s %ld %*s", title, &tmp);
	if (strcmp(title, "MemTotal:") == 0)
        {
            totalMem = tmp;
            usedMem = tmp;
            printf("total %ld\n", tmp);
        }

	while(r != EOF)
	{
	    //printf("%s %ld\n", title, tmp);
	    r = fscanf(f, "%s %ld %*s", title, &tmp);
	    if (strcmp(title, "MemTotal:") == 0)
	    {
		totalMem = tmp;
		usedMem = tmp;
		// printf("total %ld\n", tmp);
	    }
	    else if (strcmp(title, "MemFree:") == 0)
	    {
		usedMem = usedMem - tmp;
		// printf("%ld free %ld\n", totalMem, tmp);
            }
	    else if (strcmp(title, "Buffers:") == 0)
	    {
		usedMem = usedMem - tmp;
		// printf("%ld Buffer %ld\n", totalMem, tmp);
	    }
	    else if (strcmp(title, "Cached:") == 0)
	    {
		usedMem = usedMem - tmp;
		// printf("%ld cached %ld\n", totalMem, tmp);
            }
	    else if (strcmp(title, "SwapTotal:") == 0)
	    {
                swaptotal = tmp;
		// printf("%ld\n", tmp);
            }
	    else if (strcmp(title, "SwapFree:") == 0)
                swapfree = tmp;
	}
	//printf("%ld %ld %ld %ld\n", totalMem, usedMem, swaptotal, swapfree);
        fclose(f);
	if (swaptotal == swapfree)
	    swappct = 0;
	else
	    swappct = 100-(((double)(swaptotal - swapfree)/swaptotal)*100);

//        printf("Mem Stats %f %f\n", (((double)usedMem/totalMem)*100), swappct);

        snprintf(data, sizeof(data), "mem_usage,host=RPi3b value=%f\n", (((double)usedMem/totalMem)*100));
        curl = curl_easy_init();
        if(curl)
        {
            curl_easy_setopt(curl, CURLOPT_URL, "http://192.168.0.137:8086/write?db=perfdb");
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
            res = curl_easy_perform(curl);

            if(res != CURLE_OK)
                fprintf(stderr, "curl_easy_perform() for mem_usage failed: %s\n", curl_easy_strerror(res));
            else
                fprintf(stderr, "curl_easy_perform() for mem_usage succeeded");
            curl_easy_cleanup(curl);
        }

	snprintf(data, sizeof(data), "swap_usage,host=Rpi3b value=%f\n", swappct);
        curl = curl_easy_init();
        if(curl)
        {
            curl_easy_setopt(curl, CURLOPT_URL, "http://192.168.0.137:8086/write?db=perfdb");
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
            res = curl_easy_perform(curl);

            if(res != CURLE_OK)
                fprintf(stderr, "curl_easy_perform() for swp_usage failed: %s\n", curl_easy_strerror(res));
            else
                fprintf(stderr, "curl_easy_perform() for swp_usage succeeded");
            curl_easy_cleanup(curl);
        }

    }
}
