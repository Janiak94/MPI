# Assignment report on MPI (hpcgpXXX)

## Program layout

The program is implemented in a single file, with the following functions and structures

struct vertex_t
Describes a vertex. Have an array with vertices connected to the vertex, length of the array, the vertex id, if it is visited and previous vertex id.

void load_data(FILE *fp)

Master process reads and parses the file into a vertex_t structure array.
Creates process local vertex_t arrays and sends them to the other processes.
To reduce memory consumption and increase locality, each vertex in the local array (in the vertex structure) only list the connected vertices that are assigned to the process.

void update_price()
Used inside of dijkstra() to update vertex prices. We iterate over the edges from the current_vert and update the weights according to a criterion. If current_price + edge weight is less than the lowest price of this vertex we update the vertex price. If the vertex is already visited we don’t need to do anything. While updating the weights we update the local_min_weight variable if we find a vertex with a lower price.

void update_min()
Used inside of dijkstra() to get the cheapest vertex of the vertices assigned to the process. If our minimum value is dirty, indicated by local_min_is_dirty, we have to recompute the minimum price. This is done by iterating over our set of given vertices and changing the local_minimum variables accordingly. For processes not visited in this iteration we don’t need to recalculate local_min_price, saving time and system resources.

-	void set_visited()
Used inside of dijkstra() to a vertex to visited and make sure we don’t get stuck on the same vertex. This function is called after agreeing on a next vertex. If the vertex belongs to a given process, this means the local minimum from this instance was used. We therefore change the is_visited flag. This vertex is no longer a valid minimum vertex, since the vertex is now visited, we have to invalidate our minimum calculation. This is done by enabling the local_min_is_dirty flag, which tells us the local minimum is of no use. 

void dijkstra()
Dijkstras algorithm implemented with MPI. Each worker (including master) calculates the
cheapest vertex and then uses MPI_Allreduce to find the cheapest vertex. In addition to the cheapest vertex, the rank of the responsible process is shared globally. This information is used to broadcast other information using MPI_Bcast. The information shared is the next vertex as well as the previous node visited for the current node. The previous node information is calculated locally for all processes and thus needs to be shared globally in order to be able to backtrack the shortest path. If all other vertices are more expensive than the destination vertex, stop the iterations. Since we’ve already reached a higher minimum price than the destination, we know that we cannot reach a shorter path. The process that has been assigned the destination vertex traces the path and prints the result. This is done by looking at the destination’s previous vertex and then following and storing the chain. Since the chain is reverse order the path has to be temporarily stored in an array. 

void parse_args()
Parses the input arguments and does a simple error check.

int main(int argc, char *argv[])
Initializes MPI and starts dijkstra.

Except from stdlib we of course include mpi.h. We also take advantage of global variables.

#Performance

# Improvements
Using a binary heap, this however requires a hashmap in order to achieve O(1) inserts.

The program is implemented as a single main function in one file. The only required include files are standard headers:
- in- and output via the command line (printf and scanf provided by stdio),
- a source of randomness (srand and rand provided by stdlib),
- a source of time to seed the random number generator (time).

The corse layout of the main function consists of three parts:
- initalization of the random number generator and variables,
- a input-output loop providing the main functionality,
- finalization (no memory clean up is needed).

Functionality is provided by a while loop that asserts equality of two variables m and n. The former serves as the output number, the latter as the input number. Asserting equality of m and n thus implements the desired exit condition. Within the loop m is initialized as a random number modulo 1000. We take the absolute value of a random signed integer to gurantee a nonnegative result. Output and input is provided directly via printf and scanf.

## Performance

