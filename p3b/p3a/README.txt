p3a: pzip (Parallel Zip)
Shubham Singh, Yu-Lin Yang

Our initial approach was to map an entire file into memory using mmap. However, we soon realized that this was very inefficient and would not work for very large files, as some test inputs. We then decided to loop over the files and divide them up into chunks of pagesize, and then loop over the number of chunks for each file. This looked promising, however, we ran into problems with allocating space for the array to hold processed chunks. As our array was a struct consisting of an int and a char, it took up 8 bytes of memory. The array had to hold 8 bytes of each chunk of each file, and thus it would take up huge memory. Furthermore as we had several different structs and pointers and loops to iterate through them, we were failing timing quite significantly.

We then decided to change the struct array into a char array, and simply pass pointers to the int and a char. This would take up 5 bytes of space. However, we still had the issue of figuring out how much space to allocate. So we simply calculated the total space at the beginning and then did any looping or processing. This also improved timing as several computations only needed to happen once.

