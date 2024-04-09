# WebCrawler

# Run command (MAC OS using clang):
 - `clang filename.c -o filename -lcurl -lxml2`

# Problem statement:
- The internet is vast and ever-growing. Traditional single-threaded web crawlers are not
efficient enough to handle the volume of information available. The challenge is to develop
a web crawler that can navigate the web efficiently by fetching multiple pages in parallel,
thus significantly reducing the time required to crawl a set of URLs.

# Achieve the following:
1. Thread Management
• Create and manage worker threads that fetch and process web pages in parallel.
• Implement a mechanism for terminating threads gracefully once their task is
complete.
2. URL Queue
• Design a thread-safe queue that holds URLs to be crawled.
• Ensure that multiple threads can add to and fetch from the queue without causing
data corruption.
3. HTML Parsing
• Parse HTML content to extract links.
• Use a library or write a custom parser to identify and follow links within HTML
documents.
4. Depth Control
• Implement depth control to limit how deep the crawler goes into a website.
• The user should be able to specify the maximum depth as a parameter.
5. Synchronization
• Use mutexes, semaphores, or other synchronization primitives to ensure that
shared resources like the URL queue are accessed safely.
6. Error Handling
• Implement error handling to manage network failures, invalid URLs, and other
exceptions.
• Ensure that the crawler can recover from errors and continue operating.
7. Logging
• Log the progress of the web crawler, including which URLs have been visited and
any errors encountered.
• Optionally, implement a verbosity level for the logging system.