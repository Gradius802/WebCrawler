/*
Operating Systems Spring 2024
Final Project
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#define MAXLEN 1024

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

int main() {
    Queue queue;
    initializeQueue(&queue);

    const char* start_url = "https://example.com";
    enqueue(&queue, start_url);
    enqueue(&queue, start_url);
    enqueue(&queue, start_url);

    display(&queue);

    return 0;
}