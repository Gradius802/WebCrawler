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

// Maximum length of the URL queue
#define MAXLEN 1024

// Number of worker threads
#define NUMWORKERS 2

// Maximum length of the URL
#define MAX_URL_LENGTH 256

// Struct to hold the URL queue
typedef struct
{
    char* urls[MAXLEN]; // Array to store URLs
    int front, rear;    // Front and rear pointers of the queue
} Queue;

// Struct to hold CURL response
struct CURLResponse
{
    char *html;         // Pointer to HTML content
    size_t size;        // Size of HTML content
};

// Struct to pass data to worker threads
typedef struct
{
    Queue *queue;                   // Pointer to the URL queue
    pthread_mutex_t *mutex;         // Pointer to mutex for thread safety
    int maxdepth;                   // Maximum depth for URL traversal
} ThreadData;

// Function prototypes
struct CURLResponse GetRequest(CURL *curl_handle, const char *url);
static size_t WriteHTMLCallback(void *contents, size_t size, size_t nmemb, void *userp);
void extractUrls(htmlDocPtr doc, Queue* q, pthread_mutex_t *mutex);
void *worker(void *arg);
void logEvent(const char *event, const char *url, const char *status, int depth);


// QUEUE FUNCTIONS

/*
    Initialize the URL queue.

    Preconditions:  'queue' points to a valid Queue structure.
    Postcondition:  The front and rear pointers of the queue are set to -1, indicating an empty queue.
*/
void initializeQueue(Queue* queue)
{
    queue->front = queue->rear = -1; // Set front and rear pointers to -1 for an empty queue
}

/*
    Check if the URL queue is empty.

    Preconditions:  'queue' points to a valid Queue structure.
    Postcondition:  Returns 1 if the queue is empty, otherwise returns 0.
*/
int isQueueEmpty(Queue* queue)
{
    return queue->front == -1; // Return 1 if the queue is empty, otherwise return 0
}

/*
    Enqueue a URL into the URL queue.

    Preconditions:  'queue' points to a valid Queue structure, 'url' points to a valid string containing the URL to be enqueued.
    Postcondition:  If the queue is not full, the URL is enqueued at the rear of the queue.
*/
void enqueue(Queue* queue, const char* url)
{
    if (queue->rear == MAXLEN - 1)          // Check if the queue is full
    {
        printf("Queue is full!\n");         // Print message indicating the queue is full
        return;                             // Return without enqueuing
    }
    if (queue->front == -1)                 // Check if the queue is empty
    {
        queue->front = 0;                   // Set the front pointer to 0
    }
    queue->rear++;                          // Increment the rear pointer
    queue->urls[queue->rear] = strdup(url); // Enqueue the URL by copying it into the queue array
}

// Dequeue a URL from the queue
char* dequeue(Queue* queue)
{
    if (isQueueEmpty(queue))               // Check if the queue is empty
    {
        printf("Queue is empty!\n");       // Print message indicating the queue is empty
        return NULL;                       // Return NULL indicating no URL is dequeued
    }
    char* url = queue->urls[queue->front]; // Get the URL from the front of the queue
    if (queue->front == queue->rear)       // If there's only one element in the queue
    {
        queue->front = queue->rear = -1;   // Set front and rear pointers to indicate an empty queue
    }
    else
    {
        queue->front++;                    // Move the front pointer to the next element
    }
    return url;                            // Return the dequeued URL
}

/*
    Display the contents of the URL queue.

    Preconditions:  'queue' points to a valid Queue structure.
    Postcondition:  The URLs in the queue are displayed on the console.
*/
void display(Queue* queue)
{
    if (queue->rear == -1)                                // If the queue is empty
        printf("\nQueue is Empty!");                      // Print message indicating the queue is empty
    else
    {
        printf("\nQueue elements are:\n");                // Print message indicating the queue elements will be displayed
        for (int i = queue->front; i <= queue->rear; i++) // Iterate over the elements in the queue
            printf("%s  ", queue->urls[i]);               // Print each URL in the queue
    }
    printf("\n");                                         // Print a newline character
}

// END QUEUE FUNCTIONS

// Create a mutex for the URL queue
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER; // Initialize mutex for queue synchronization

// Global CURL handle
CURL *curl_handle; // Global variable to hold the CURL handle

// Main function
int main()
{
    // Initialize CURL
    curl_global_init(CURL_GLOBAL_ALL); // Initialize CURL library
    curl_handle = curl_easy_init();    // Initialize CURL handle

    //Initial entry in to the log file,
    logEvent("Program initialization", "", NULL, -2);

    // Setup URL queue
    Queue *q = malloc(sizeof(Queue));                   // Allocate memory for the URL queue
    if (q == NULL)                                      // If memory allocation fails
    {
        fprintf(stderr, "Memory allocation failed!\n"); // Print error message
    }
    initializeQueue(q);                                 // Initialize the URL queue

    int MAX_DEPTH = 0;                                  // Variable to store maximum depth

    //loop until user enter -1 for depth
    while(MAX_DEPTH != -1)
    {
        printf("Enter a depth for searching: (Enter -1 to exit)\n");          // Prompt user to enter depth for searching
        scanf("%d", &MAX_DEPTH);                                              // Read user input for maximum depth

        if(MAX_DEPTH == -1)
        {
            break; //exit the loop.
        }

        // Set the initial URL to parse
        char initialURL[MAX_URL_LENGTH];                     // Variable to store the user-inputted URL
        printf("Enter the initial URL to parse: ");          // Prompt the user to enter the initial URL
        scanf("%s", initialURL);                             // Read the user input for the initial URL

        pthread_mutex_lock(&queue_mutex);                    // Acquire mutex lock
        enqueue(q, initialURL);                              // Enqueue the initial URL provided by the user
        pthread_mutex_unlock(&queue_mutex);                  // Release mutex lock

        // Setup threads
        pthread_t threads[NUMWORKERS];                          // Array to hold worker thread IDs
        ThreadData thread_data = {q, &queue_mutex, MAX_DEPTH }; // Create thread data structure with queue, mutex, and max depth

        // Create worker threads
        for (int i = 0; i < NUMWORKERS; i++)                // Iterate over number of worker threads
        {
            if (pthread_create(&threads[i], NULL, worker, &thread_data) != 0) // Create a worker thread
            {
                perror("Error creating thread");            // Print error message if thread creation fails
                return 0;                                   // Return from main with error code
            }
        }

        // Wait for worker threads to finish
        for (int i = 0; i < NUMWORKERS; i++)                // Iterate over worker threads
        {
            pthread_join(threads[i], NULL);                 // Wait for a worker thread to finish
        }
    }

    // Cleanup CURL instance
    curl_easy_cleanup(curl_handle);                     // Cleanup CURL handle
    curl_global_cleanup();                              // Cleanup CURL library

    //add log for end of program
    logEvent("Program Terminated", "", NULL, -1);

    return 0; // Return from main with success code
}

/*
    Curl callback function for writing HTML content.

    Description:
    This function is called by libcurl when HTML content is received during a HTTP request. It reallocates memory
    for the HTML content, copies the received data, and updates the size of the content.

    Preconditions:
    'contents' must point to the received data.
    'size' and 'nmemb' specify the size of each data element and the number of elements.
    'userp' must point to a valid struct CURLResponse containing the HTML content and its size.

    Postcondition:
    The HTML content is reallocated with additional memory if needed, the received data is copied to the content buffer,
    and the size of the content is updated accordingly. The function returns the size of the received data.
*/
static size_t WriteHTMLCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;                                        // Calculate the real size of the data
    struct CURLResponse *mem = (struct CURLResponse *)userp;               // Cast user pointer to CURL response structure
    char *ptr = realloc(mem->html, mem->size + realsize + 1);              // Reallocate memory for HTML content

    if (!ptr)                                                              // If reallocation fails
    {
        printf("Not enough memory available (realloc returned NULL)\n");   // Print error message
        return 0;                                                          // Return 0 indicating failure
    }

    mem->html = ptr;                                                       // Update HTML pointer to the reallocated memory
    memcpy(&(mem->html[mem->size]), contents, realsize);                   // Copy contents to HTML memory
    mem->size += realsize;                                                 // Update the size of HTML content
    mem->html[mem->size] = 0;                                              // Null-terminate the HTML content

    return realsize;                                                       // Return the real size of the data
}

/*
    Perform a GET request to retrieve an HTML document from a specified URL.

    Description:
    This function sends a GET request to the provided URL using libcurl and retrieves the HTML content of the webpage.
    The retrieved HTML content is stored in a struct CURLResponse.

    Preconditions:
    'curl_handle' must point to a valid CURL handle initialized by curl_easy_init().
    'url' must point to a valid null-terminated string containing the URL to retrieve.
    Memory allocation for 'response.html' must be handled before calling this function.

    Postcondition:
    The function returns a struct CURLResponse containing the HTML content retrieved from the URL.
    If the request fails, an error message is printed.
*/
struct CURLResponse GetRequest(CURL *curl_handle, const char *url)
{
    CURLcode res;                   // Variable to store curl result

    // Setup response
    struct CURLResponse response;   // Create response structure
    response.html = malloc(1);      // Allocate memory for HTML content
    response.size = 0;              // Initialize size to 0

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

/*
    Extract URLs from the anchor tags within the HTML document.

    Description:
    This function traverses the XML document tree starting from the specified node (doc->children).
    For each node encountered, it checks if it represents an anchor tag (<a>) and extracts the URL from its href attribute.
    If the URL starts with "http://" or "https://", it is considered a valid link and enqueued into the provided queue.
    The function recursively explores child nodes to ensure all anchor tags within the document are processed.

    Preconditions:
    'doc' must point to a valid htmlDocPtr representing the HTML document.
    'q' must point to a valid Queue structure initialized with URLs to enqueue extracted URLs.
    'mutex' must point to a valid pthread_mutex_t used for queue synchronization.

    Postcondition:
    The URLs extracted from the anchor tags within the HTML document are enqueued into the provided queue.
*/
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

/*
    Worker function responsible for processing URLs.

    Description:
    This function processes URLs by dequeuing them from the provided queue and fetching their HTML content.
    It continues to process URLs until either the queue becomes empty or the maximum depth is reached.
    For each URL, it retrieves the HTML content, parses it to extract URLs, and enqueues the extracted URLs for further processing.
    The function operates within a loop, continuously dequeuing URLs and processing them until the termination condition is met.

    Preconditions:
    'arg' must point to a valid ThreadData structure containing the queue pointer, mutex pointer, and maximum depth.
    'queue' must point to a valid Queue structure initialized with URLs to be processed.
    'mutex' must point to a valid pthread_mutex_t used for queue synchronization.
    'MAX_DEPTH' must be an integer representing the maximum depth of URL processing.

    Postcondition:
    The function processes URLs from the queue, fetching their HTML content and enqueuing extracted URLs for further processing.
    It terminates when either the queue becomes empty or the maximum depth is reached.
*/
void *worker(void *arg)
{

    // Setup data
    ThreadData *data = (ThreadData *) arg;      // Cast argument to thread data structure
    Queue *queue = data->queue;                 // Get the queue pointer
    pthread_mutex_t *mutex = data->mutex;       // Get the mutex pointer
    int MAX_DEPTH = data->maxdepth;             // Get the maximum depth

    int curdepth = 0;                           // Initialize current depth

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

        // Log the start of processing for the URL
        logEvent("Processing URL", url, NULL, curdepth);

        struct CURLResponse response = GetRequest(curl_handle, url);   // Get HTML response for the URL

        // Check for response
        if (response.html)   // If HTML content is received
        {
            htmlDocPtr doc = htmlReadMemory(response.html, (unsigned long)response.size, NULL, NULL, HTML_PARSE_NOERROR);   // Parse HTML document
            if (doc)   // If document parsing is successful
            {
                // Log the successful retrieval of HTML content
                logEvent("HTML content retrieved", url, NULL, curdepth);

                extractUrls(doc, queue, mutex);   // Extract URLs from the document
                xmlFreeDoc(doc);   // Free parsed HTML document
            }
            free(response.html);   // Free HTML content memory
        }
        else
        {
            // Log failure to retrieve the HTML content
            logEvent("Failed to retrieve HTML content", url, NULL, curdepth);
        }

        //Finished processing URLs, log to file.
        logEvent("Finished processing URL", url, NULL, curdepth);

        free(url); // Free the URL string
        curdepth++;   // Increment current depth
    }
    return NULL;   // Return from worker thread
}

/*
    logEvent

    Description: this function logs information to a log file.
    If the log file does not exist yet, it is created. Subsequent calls append
    new log entries to the existing log file.

    Preconditions/parameters.
    - event: The event or action being logged.
    - url: The URL associated with the event.
    - status: The status of the event (optional).
    - depth: The depth of the event in the URL traversal.

    Postconditions:
    - If the log file does not exist yet, it is created and the log entry is written.
    - If the log file already exists, the log entry is appended to the existing file.
*/
void logEvent(const char *event, const char *url, const char *status, int depth)
{
    // Open the log file in read mode to check if it exists
    FILE *logFile = fopen("crawler.log", "r");
    //if depth == -2, we are on the initial run of the program
    if (logFile == NULL || depth == -2)
    {
        // If the log file does not exist yet, create it in write mode
        logFile = fopen("crawler.log", "w");
    }
    else
    {
        // If the log file already exists, close it and open it in append mode
        fclose(logFile);
        logFile = fopen("crawler.log", "a");
    }

    if (logFile == NULL)
    {
        fprintf(stderr, "Error opening log file\n");
        return;
    }

    // Get the current time
    // Declare a variable to hold the current time
    time_t currentTime;
    // Declare a variable to hold the local time structure
    struct tm *localTime;
    // Declare a string to store the formatted time
    char timeString[20];

    // Get the current time in seconds
    currentTime = time(NULL);
    // Convert the current time to the local time
    localTime = localtime(&currentTime);
    // Format the local time as a string in the format "YYYY-MM-DD HH:MM:SS"
    strftime(timeString, sizeof(timeString), "%Y-%m-%d %H:%M:%S", localTime);


    // Write the log entry to the log file
    fprintf(logFile, "[%s] %s: %s\n", timeString, event, url);

    // If status is provided, log it as well
    if (status != NULL)
    {
        fprintf(logFile, "\tStatus: %s\n", status);
    }

    // If depth is provided, log it as well
    if (depth >= 0)
    {
        fprintf(logFile, "\tDepth: %d\n", depth);
    }

    // Close the log file
    fclose(logFile);
}
