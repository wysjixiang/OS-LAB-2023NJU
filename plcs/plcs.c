#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "thread.h"
#include "thread-sync.h"

#define MAXN 10000
int T, N, M;
char A[MAXN + 1], B[MAXN + 1];
int dp[MAXN][MAXN];
int result;

#define DP(x, y) (((x) >= 0 && (y) >= 0) ? dp[x][y] : 0)
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MAX3(x, y, z) MAX(MAX(x, y), z)

spinlock_t lk;

void Tworker(int id) {
	 if (id != 1) {
    // This is a serial implementation
    // Only one worker needs to be activated
    return;
  }

  for (int i = 0; i < N; i++) {
    for (int j = 0; j < M; j++) {
      // Always try to make DP code more readable
      int skip_a = DP(i - 1, j);
      int skip_b = DP(i, j - 1);
      int take_both = DP(i - 1, j - 1) + (A[i] == B[j]);
      dp[i][j] = MAX3(skip_a, skip_b, take_both);
    }
  }

  result = dp[N - 1][M - 1];
}

int main(int argc, char *argv[]) {
  // No need to change
  assert(scanf("%s%s", A, B) == 2);
  N = strlen(A);
  M = strlen(B);
  T = !argv[1] ? 1 : atoi(argv[1]);
  printf("Thread num = %d\n",T);

  // pre-process
  // no need since dp is defined in static area. so dp[][] are set zeros;


  // Add preprocessing code here

  for (int i = 0; i < T; i++) {
    create(Tworker);
  }
  join();  // Wait for all workers }

  printf("result = %d\n", dp[M][N]);

}




void *computeBlock(void * myid) {

	int id = *(int *)myid ;
	int i,d,j,r,c,rSize,cSize,addR,addC;

	d = 0;
    	for (r = id; r < num_blocksY; r = r + NUM_THREADS) {
		addR = r * SUBY_SIZE;
		rSize = strlen(subY[r]);
		//printf ("at %d NEW - %s\n",id,subY[r]);

		for (c = 0; c <= num_blocksX &&
				d < (num_blocksY+num_blocksX-1) ; c++) { 
	
			if (c == num_blocksX && d < (num_blocksY-1)) {
				break;
			}
			else if (c == num_blocksX && d >= (num_blocksY-1)) {
				pthread_barrier_wait(&barr);
				d++;
				c--;
				continue;
			}
			while (c > (d-r) ) {
				pthread_barrier_wait(&barr);
				d++;
	//			printf("Thread %d agree %d\n",r,d);
			}

			cSize = strlen(subX[c]);
			addC = c * SUBMAT_SIZE;
		//	printf ("at %d and - %s\n",id,subX[c]);

			for (i = 0; i < rSize; i++) {

				for (j = 0; j < cSize ; j++) {

					if (subX[c][j] == subY[r][i]) {
						fTab[i+addR+1][j+addC+1] =
				       			fTab[i+addR][j+addC] + 1;
					}	
					else {
						fTab[i+addR+1][j+addC+1] =
							util_max(fTab[i+addR][j+addC+1],
								fTab[i+addR+1][j+addC]);
					}
				}
			}	
   		}
	}
 	pthread_exit(NULL);
}


