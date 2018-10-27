# Assignment report on MPI (hpcgpXXX)

## Program layout

The program is implemented in a single file, with the following functions and structures

- struct vertex_t

Describes a vertex. Have an array with vertices connected to the vertex, length of the array, the vertex id, if it is visited and previous vertex id.

- void load_data(FILE *fp)

Master process reads and parses the file into a vertex_t structure array.
Creates process local vertex_t arrays and sends them to the other processes.
To reduce memory consumption and increase locality, each vertex in the local array (in the vertex structure) only list the connected vertices that are assigned to the process.

- void update_price()

Used inside of dijkstra() to update vertex prices.

- void update_min()

Used inside of dijkstra() to get the cheapest vertex of the vertices assigned to the process.

-	void set_visited()

Used inside of dijkstra() to a vertex to visited and make sure we don’t get stuck on the same vertex.

- void dijkstra()

Dijkstras algorithm implemented with MPI. Each worker (including master) calculates the
cheapest vertex and then uses MPI_Allreduce to find the cheapest vertex. If all other vertices
are more expensive than the destination vertex, stop the iterations. The process that has been assigned the destination vertex traces the path and prints the result.

- void parse_args()

Parses the input arguments and does a simple error check.

- int main(int argc, char *argv[])

Initializes MPI and starts dijkstra.


Except from stdlib we of course include mpi.h. We also take advantage of global variables.

## Performance

WE CRUSH YOU RAUM
