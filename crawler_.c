/*
Operating Systems Spring 2024
Final Project
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#define MAXLEN 1024

//function prototypes
struct CURLResponse GetRequest(CURL *curl_handle, const char *url);
static size_t WriteHTMLCallback(void *contents, size_t size, size_t nmemb, void *userp);


//Struct to hold our QUEUE
typedef struct {
    char* urls[MAXLEN];
    int front, rear;
} Queue;

//Initialize queue
void initializeQueue(Queue* queue) {
    queue->front = queue->rear = -1;
}

//Check if queue is empty
int isQueueEmpty(Queue* queue) {
    return queue->front == -1;
}

//Enqueue
void enqueue(Queue* queue, const char* url) {
    if (queue->rear == MAXLEN - 1) {
        printf("Queue is full!\n");
        return;
    }
    if (queue->front == -1) {
        queue->front = 0;
    }
    queue->rear++;
    queue->urls[queue->rear] = strdup(url);
}

//Dequeue
char* dequeue(Queue* queue) {
    if (isQueueEmpty(queue)) {
        printf("Queue is empty!\n");
        return NULL;
    }
    char* url = queue->urls[queue->front];
    //check if queue is empty
    if (queue->front == queue->rear) {
        queue->front = queue->rear = -1;
    } else {
        queue->front++;
    }
    return url;
}

//Display queue
void display(Queue* queue) {
  if (queue->rear == -1)
    printf("\nQueue is Empty!");
  else {
    printf("\nQueue elements are:\n");
    for (int i = queue->front; i <= queue->rear; i++)
      printf("%s  ", queue->urls[i]);
  }
  printf("\n");
}

//Curl response
struct CURLResponse
{
    char *html;
    size_t size;
};

    int main() {
        //initialize curl
        curl_global_init(CURL_GLOBAL_ALL);
        CURL *curl_handle = curl_easy_init();
    
        //retrieve the html doc
        struct CURLResponse response = GetRequest(curl_handle, "http://example.com");

        //print the HTML 
        printf("%s\n", response.html);

        //logic to handle html and look for URLS
        /*
        todo
        */
    
        //cleanup curl instance
        curl_easy_cleanup(curl_handle);
        curl_global_cleanup();
    
        return 0;
    }

//Curl callback
static size_t WriteHTMLCallback(void *contents, size_t size, size_t nmemb, void *userp)
    {
        size_t realsize = size * nmemb;
        struct CURLResponse *mem = (struct CURLResponse *)userp;
        char *ptr = realloc(mem->html, mem->size + realsize + 1);
      
        if (!ptr)
        {
            printf("Not enough memory available (realloc returned NULL)\n");
            return 0;
        }
      
        mem->html = ptr;
        memcpy(&(mem->html[mem->size]), contents, realsize);
        mem->size += realsize;
        mem->html[mem->size] = 0;
      
        return realsize;
    }

//Get HTML Document
struct CURLResponse GetRequest(CURL *curl_handle, const char *url)
    {
        CURLcode res;

        //settup response
        struct CURLResponse response;
        response.html = malloc(1);
        response.size = 0;
      
        //initalize URL to GET
        curl_easy_setopt(curl_handle, CURLOPT_URL, url);

        //send data to callback
        curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteHTMLCallback);
        curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&response);

        //set headers
        curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/117.0.0.0 Safari/537.36");

        //preform request
        res = curl_easy_perform(curl_handle);
      
        //check for error
        if (res != CURLE_OK)
        {
            fprintf(stderr, "GET request failed: %s\n", curl_easy_strerror(res));
        }
      
        return response;
    }