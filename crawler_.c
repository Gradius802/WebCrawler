/*
Operating Systems Spring 2024
Final Project
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <pthread.h>
#include <libxml/HTMLparser.h>

//
#define MAXLEN 1024
#define NUMWORKERS 2


// Struct to hold our QUEUE
typedef struct {
    char* urls[MAXLEN];
    int front, rear;
} Queue;

// Struct to hold curl reponse
struct CURLResponse
{
    char *html;
    size_t size;
};

// Struct to pass data to workers
typedef struct {
    Queue *queue;
    pthread_mutex_t *mutex;
    int maxdepth;
} ThreadData;


// function prototypes
struct CURLResponse GetRequest(CURL *curl_handle, const char *url);
static size_t WriteHTMLCallback(void *contents, size_t size, size_t nmemb, void *userp);
void extractUrls(htmlDocPtr doc, Queue* q, pthread_mutex_t *mutex);
void *worker(void *arg);

// QUEUE FUNCTIONS

// Initialize queue
void initializeQueue(Queue* queue) {
    queue->front = queue->rear = -1;
}

// Check if queue is empty
int isQueueEmpty(Queue* queue) {
    return queue->front == -1;
}

// Enqueue
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

// Dequeue
char* dequeue(Queue* queue) {
    if (isQueueEmpty(queue)) {
        printf("Queue is empty!\n");
        return NULL;
    }
    char* url = queue->urls[queue->front];
    // check if queue is empty
    if (queue->front == queue->rear) {
        queue->front = queue->rear = -1;
    } else {
        queue->front++;
    }
    return url;
}

// Display queue
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

// END QUEUE FUNCTIONS

// create a MUTEX for our queue
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;

// create global curl handle
CURL *curl_handle;

int main() {
    // initialize curl
    curl_global_init(CURL_GLOBAL_ALL);
    curl_handle = curl_easy_init();

    // settup queue
    Queue *q = malloc(sizeof(Queue));
    if (q == NULL) {
        fprintf(stderr, "Memory allocation failed!\n");
    }
    initializeQueue(q); 

    int MAX_DEPTH = 0;

    printf("Enter a depth for searching: \n");
    scanf("%d", &MAX_DEPTH);

    // setup our threads
    pthread_t threads[NUMWORKERS];
    ThreadData thread_data = {q, &queue_mutex, MAX_DEPTH };


    // logic to handle html and look for URLS

    // Create worker threads
    for (int i = 0; i < NUMWORKERS; i++)
    {
        if (pthread_create(&threads[i], NULL, worker, &thread_data) != 0)
        {
            perror("Error creating thread");
            return 0;
        }
    }

    // Set the first URL to parse (TO BE CHANGED -> URL GIVEN BY USER)
    pthread_mutex_lock(&queue_mutex);
    enqueue(q, "https://www.example.com");
    pthread_mutex_unlock(&queue_mutex);

    // Wait for worker threads to finish
    for (int i = 0; i < NUMWORKERS; i++) {
        pthread_join(threads[i], NULL);
    }

    
    // cleanup curl instance
    curl_easy_cleanup(curl_handle);
    curl_global_cleanup();

    return 0;
}

// Curl callback
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

// Get HTML Document
struct CURLResponse GetRequest(CURL *curl_handle, const char *url)
    {
        CURLcode res;

        // settup response
        struct CURLResponse response;
        response.html = malloc(1);
        response.size = 0;
      
        // initalize URL to GET
        curl_easy_setopt(curl_handle, CURLOPT_URL, url);

        // send data to callback
        curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteHTMLCallback);
        curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&response);

        // set headers (mimic user)
        curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/117.0.0.0 Safari/537.36");

        // preform request
        res = curl_easy_perform(curl_handle);
      
        // check for error
        if (res != CURLE_OK)
        {
            fprintf(stderr, "GET request failed: %s\n", curl_easy_strerror(res));
        }
      
        return response;
    }


// Extract urls from current node
void extractUrls(htmlDocPtr doc, Queue* q, pthread_mutex_t *mutex) {
    // set anchor
    xmlNode *aNode = doc->children;

    // check nodes
    for (; aNode; aNode = aNode->next) {
        // check if node is an <a> tag
        if (aNode->type == XML_ELEMENT_NODE && !xmlStrcmp(aNode->name, (const xmlChar *)"a"))
        {
            xmlChar *href = xmlGetProp(aNode, (const xmlChar *)"href");
            if (href) 
            {
                // Filter for actual URLS
                if (strncmp((const char *)href, "http://", 7) == 0 || 
                    strncmp((const char *)href, "https://", 8) == 0) 
                {
                    // Make sure we are linking to a specific part of the document
                    if (href[0] != '#') {
                        printf("%s\n\n", href);
                        pthread_mutex_lock(mutex);
                        enqueue(q, href); 
                        pthread_mutex_unlock(mutex);
                    }
                }
                xmlFree(href);
            }
        }

        // search child's children nodes
        extractUrls(aNode, q, mutex); 
    }
}

void *worker(void *arg) {

    // Settup data
    ThreadData *data = (ThreadData *) arg;
    Queue *queue = data->queue;
    pthread_mutex_t *mutex = data->mutex;
    int MAX_DEPTH = data->maxdepth;

    int curdepth = 0;

    // loop through all documents given by urls until there is an empty queue or depth limit is reached
    while (1) {
        char *url;

        pthread_mutex_lock(mutex);
        url = dequeue(queue);
        pthread_mutex_unlock(mutex);

        if (url == NULL) {  // Check if queue was empty
            break; 
        }
        if(curdepth >= MAX_DEPTH){
            continue;
        }

        struct CURLResponse response = GetRequest(curl_handle, url);

        // check for response
        if (response.html) { 
            htmlDocPtr doc = htmlReadMemory(response.html, (unsigned long)response.size, NULL, NULL, HTML_PARSE_NOERROR);
            if (doc) {
                extractUrls(doc, queue, mutex);
                xmlFreeDoc(doc);
            }
            free(response.html); 
        }
        free(url); // Free the URL string
        curdepth++;
    }
    return NULL;
}