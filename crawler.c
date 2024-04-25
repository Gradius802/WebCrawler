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

#define MAXLEN 1024
#define NUMWORKERS 2


// Struct to hold our QUEUE
typedef struct
{
    char* urls[MAXLEN]; // Array to store URLs
    int front, rear;    // Pointers to the front and rear of the queue
} Queue;

// Struct to hold curl response
struct CURLResponse
{
    char
    *html;             // Pointer to the HTML content
    size_t size;       // Size of the HTML content
};

// Struct to pass data to workers
typedef struct
{
    Queue *queue;                    // Pointer to the queue
    pthread_mutex_t *mutex;         // Pointer to the mutex
    int maxdepth;                   // Maximum depth for searching
} ThreadData;


// Function prototypes
struct CURLResponse GetRequest(CURL *curl_handle, const char *url);  // Function to perform GET request
static size_t WriteHTMLCallback(void *contents, size_t size, size_t nmemb, void *userp); // Callback function to write HTML content
void extractUrls(htmlDocPtr doc, Queue* q, pthread_mutex_t *mutex);  // Function to extract URLs from HTML document
void *worker(void *arg);  // Worker function to process URLs


// QUEUE FUNCTIONS

// Initialize queue
void initializeQueue(Queue* queue)
{
    queue->front = queue->rear = -1;  // Set front and rear pointers to indicate an empty queue
}

// Check if queue is empty
int isQueueEmpty(Queue* queue)
{
    return queue->front == -1;   // Return true if the front pointer is -1, indicating an empty queue
}

// Enqueue
void enqueue(Queue* queue, const char* url)
{
    if (queue->rear == MAXLEN - 1)  // If rear pointer reaches the end of the array
    {
        printf("Queue is full!\n");  // Print message indicating the queue is full
        return;  // Return without enqueueing
    }
    if (queue->front == -1)   // If the queue is empty
    {
        queue->front = 0;   // Set front pointer to 0
    }
    queue->rear++;   // Increment rear pointer
    queue->urls[queue->rear] = strdup(url);  // Enqueue the URL by copying it into the queue array
}

// Dequeue
char* dequeue(Queue* queue)
{
    if (isQueueEmpty(queue))   // If the queue is empty
    {
        printf("Queue is empty!\n");  // Print message indicating the queue is empty
        return NULL;  // Return NULL indicating no URL is dequeued
    }
    char* url = queue->urls[queue->front];  // Get the URL from the front of the queue
    if (queue->front == queue->rear)  // If there's only one element in the queue
    {
        queue->front = queue->rear = -1;  // Set front and rear pointers to indicate an empty queue
    }
    else
    {
        queue->front++;   // Move the front pointer to the next element
    }
    return url;   // Return the dequeued URL
}

// Display queue
void display(Queue* queue)
{
    if (queue->rear == -1)   // If the queue is empty
        printf("\nQueue is Empty!");   // Print message indicating the queue is empty
    else
    {
        printf("\nQueue elements are:\n");  // Print message indicating the queue elements will be displayed
        for (int i = queue->front; i <= queue->rear; i++)   // Iterate over the elements in the queue
            printf("%s  ", queue->urls[i]);   // Print each URL in the queue
    }
    printf("\n");   // Print a newline character
}

// END QUEUE FUNCTIONS

// Create a MUTEX for our queue
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;  // Initialize mutex for queue synchronization

// Create global curl handle
CURL *curl_handle;  // Global variable to hold the curl handle

int main()
{
    // Initialize curl
    curl_global_init(CURL_GLOBAL_ALL);   // Initialize curl library
    curl_handle = curl_easy_init();      // Initialize curl handle

    // Set up queue
    Queue *q = malloc(sizeof(Queue));   // Allocate memory for the queue
    if (q == NULL)   // If memory allocation fails
    {
        fprintf(stderr, "Memory allocation failed!\n");   // Print error message
    }
    initializeQueue(q);   // Initialize the queue

    int MAX_DEPTH = 0;   // Variable to store maximum depth

    printf("Enter a depth for searching: \n");   // Prompt user to enter depth for searching
    scanf("%d", &MAX_DEPTH);   // Read user input for maximum depth

    // Setup our threads
    pthread_t threads[NUMWORKERS];   // Array to hold worker thread IDs
    ThreadData thread_data = {q, &queue_mutex, MAX_DEPTH };   // Create thread data structure with queue, mutex, and max depth

    // Create worker threads
    for (int i = 0; i < NUMWORKERS; i++)   // Iterate over number of worker threads
    {
        if (pthread_create(&threads[i], NULL, worker, &thread_data) != 0)   // Create a worker thread
        {
            perror("Error creating thread");   // Print error message if thread creation fails
            return 0;   // Return from main with error code
        }
    }

    // Set the first URL to parse (TO BE CHANGED -> URL GIVEN BY USER)
    pthread_mutex_lock(&queue_mutex);   // Acquire mutex lock
    enqueue(q, "https://www.example.com");   // Enqueue the initial URL
    pthread_mutex_unlock(&queue_mutex);   // Release mutex lock

    // Wait for worker threads to finish
    for (int i = 0; i < NUMWORKERS; i++)   // Iterate over worker threads
    {
        pthread_join(threads[i], NULL);   // Wait for a worker thread to finish
    }

    // Cleanup curl instance
    curl_easy_cleanup(curl_handle);   // Cleanup curl handle
    curl_global_cleanup();   // Cleanup curl library

    return 0;   // Return from main with success code
}

// Curl callback
static size_t WriteHTMLCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;   // Calculate the real size of the data
    struct CURLResponse *mem = (struct CURLResponse *)userp;   // Cast user pointer to CURL response structure
    char *ptr = realloc(mem->html, mem->size + realsize + 1);   // Reallocate memory for HTML content

    if (!ptr)   // If reallocation fails
    {
        printf("Not enough memory available (realloc returned NULL)\n");   // Print error message
        return 0;   // Return 0 indicating failure
    }

    mem->html = ptr;   // Update HTML pointer to the reallocated memory
    memcpy(&(mem->html[mem->size]), contents, realsize);   // Copy contents to HTML memory
    mem->size += realsize;   // Update the size of HTML content
    mem->html[mem->size] = 0;   // Null-terminate the HTML content

    return realsize;   // Return the real size of the data
}

// Get HTML Document
struct CURLResponse GetRequest(CURL *curl_handle, const char *url)
{
    CURLcode res;   // Variable to store curl result

    // Setup response
    struct CURLResponse response;   // Create response structure
    response.html = malloc(1);   // Allocate memory for HTML content
    response.size = 0;   // Initialize size to 0

    // Initialize URL to GET
    curl_easy_setopt(curl_handle, CURLOPT_URL, url);

    // Send data to callback
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteHTMLCallback);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&response);

    // Set headers (mimic user)
    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/117.0.0.0 Safari/537.36");

    // Perform request
    res = curl_easy_perform(curl_handle);   // Perform the curl request

    // Check for error
    if (res != CURLE_OK)   // If curl operation fails
    {
        fprintf(stderr, "GET request failed: %s\n", curl_easy_strerror(res));   // Print error message
    }

    return response;   // Return the response structure
}

// Extract URLs from current node
void extractUrls(htmlDocPtr doc, Queue* q, pthread_mutex_t *mutex)
{
    // Set anchor
    xmlNode *aNode = doc->children;   // Get the first child node of the HTML document

    // Check nodes
    for (; aNode; aNode = aNode->next)   // Iterate over child nodes
    {
        // Check if node is an <a> tag
        if (aNode->type == XML_ELEMENT_NODE && !xmlStrcmp(aNode->name, (const xmlChar *)"a"))   // If node is an anchor tag
        {
            xmlChar *href = xmlGetProp(aNode, (const xmlChar *)"href");   // Get the value of the href attribute
            if (href)   // If href attribute exists
            {
                // Filter for actual URLs
                if (strncmp((const char *)href, "http://", 7) == 0 ||
                        strncmp((const char *)href, "https://", 8) == 0)   // If URL starts with "http://" or "https://"
                {
                    // Make sure we are linking to a specific part of the document
                    if (href[0] != '#')   // If href doesn't start with '#'
                    {
                        printf("%s\n\n", href);   // Print the URL
                        pthread_mutex_lock(mutex);   // Acquire mutex lock
                        enqueue(q, href);   // Enqueue the URL
                        pthread_mutex_unlock(mutex);   // Release mutex lock
                    }
                }
                xmlFree(href);   // Free the href memory
            }
        }

        // Search child's children nodes recursively
        extractUrls(aNode, q, mutex);   // Recursively call extractUrls on child nodes
    }
}

// Worker function to process URLs
void *worker(void *arg)
{

    // Setup data
    ThreadData *data = (ThreadData *) arg;   // Cast argument to thread data structure
    Queue *queue = data->queue;    // Get the queue pointer
    pthread_mutex_t *mutex = data->mutex;   // Get the mutex pointer
    int MAX_DEPTH = data->maxdepth;   // Get the maximum depth

    int curdepth = 0;   // Initialize current depth

    // Loop through all documents given by URLs until there is an empty queue or depth limit is reached
    while (1)
    {
        char *url;   // Variable to store URL

        pthread_mutex_lock(mutex);   // Acquire mutex lock
        url = dequeue(queue);   // Dequeue a URL
        pthread_mutex_unlock(mutex);   // Release mutex lock

        if (url == NULL)    // Check if queue was empty
        {
            break;   // Exit the loop if the queue is empty
        }
        if(curdepth >= MAX_DEPTH)   // Check if current depth exceeds maximum depth
        {
            continue;   // Skip processing the URL if depth limit is reached
        }

        struct CURLResponse response = GetRequest(curl_handle, url);   // Get HTML response for the URL

        // Check for response
        if (response.html)   // If HTML content is received
        {
            htmlDocPtr doc = htmlReadMemory(response.html, (unsigned long)response.size, NULL, NULL, HTML_PARSE_NOERROR);   // Parse HTML document
            if (doc)   // If document parsing is successful
            {
                extractUrls(doc, queue, mutex);   // Extract URLs from the document
                xmlFreeDoc(doc);   // Free parsed HTML document
            }
            free(response.html);   // Free HTML content memory
        }
        free(url); // Free the URL string
        curdepth++;   // Increment current depth
    }
    return NULL;   // Return from worker thread
}

