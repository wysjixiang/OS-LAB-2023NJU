#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>
#include "thread.h"
#include "thread-sync.h"

#define MAXN 10000
int T, N, M;
char A[MAXN + 1], B[MAXN + 1];
int dp[MAXN][MAXN];
bool dp_cal[MAXN][MAXN];
int result;
int dp_ref[MAXN][MAXN];


#define DP(x, y) (((x) >= 0 && (y) >= 0) ? dp[x][y] : 0)
#define DP_REF(x, y) (((x) >= 0 && (y) >= 0) ? dp_ref[x][y] : 0)
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MAX3(x, y, z) MAX(MAX(x, y), z)

spinlock_t lk;
mutex_t lk_mutex;
static int round = 1;

void Tworker(int id) {

	if(id > round) 
		return ;

	int offset = 0;
	int i,j;
	if(round > N){
		i = N - 1;
		j = round - N;
	} else{
		i = round - 1;
		j = 0;
	}

	while( i >= 0 && j < M){
		// get the lock and check if need calculation
		spin_lock(&lk);
		if(dp_cal[i][j] == 0){
			dp_cal[i][j] = 1;
			spin_unlock(&lk);
			// Always try to make DP code more readable
			int skip_a = DP(i - 1, j);
			int skip_b = DP(i, j - 1);
			int take_both = DP(i - 1, j - 1) + (A[i] == B[j]);
			dp[i][j] = MAX3(skip_a, skip_b, take_both);
		} else {
			spin_unlock(&lk);
		}
		i--;
		j++;
	}
}

void ref_dp(){

	for(int i =0; i< N; i++){
		for(int j = 0; j < M; j++){
			// Always try to make DP code more readable
			int skip_a = DP_REF(i - 1, j);
			int skip_b = DP_REF(i, j - 1);
			int take_both = DP_REF(i - 1, j - 1) + (A[i] == B[j]);
			dp_ref[i][j] = MAX3(skip_a, skip_b, take_both);
		}
	}
	return;
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
  for(int j = 0; j < N+ M - 1; j++){
	for (int i = 0; i < T; i++) {
		create(Tworker);
	}
	join();  // Wait for all workers }
	round++;
  }


	for(int i = 0;i < N; i++){
		for(int j = 0; j < M; j++){
			printf("%d ",dp[i][j]);
		}
		printf("\n");
	}

	printf("result = %d\n", dp[N-1][M-1]);

	ref_dp();
	for(int i = 0;i < N; i++){
		for(int j = 0; j < M; j++){
			printf("%d ",dp_ref[i][j]);
		}
		printf("\n");
	}
	printf("ref_result = %d\n", dp_ref[N-1][M-1]);

}

