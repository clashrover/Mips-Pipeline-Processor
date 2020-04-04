#include<bits/stdc++.h>
using namespace std;

int main(){
	int n;
	cin >>n;
	vector <int> seq;
	int sum=0;
	while(n--){
		int x ; cin >> x;
		sum = sum+x;
		seq.push_back(sum);
		// cout << sum << " ";
	}
	// cout << endl;
	vector<pair<int,int>> v;
	int i=0;
	int j=0;
	int min = seq[0];
	int max = seq[0];
	for(int k=1;k<seq.size();k++){
		if(seq[k]<min){
			pair<int,int> p; p.first = i; p.second = j;
			v.push_back(p);
			i=k;j=k;
			min = seq[k]; max=seq[k];
		}else if(seq[k]>min){
			if(seq[k]>max){
				j=k;
				max = seq[k];
			}else if(seq[k]==max){
				j=k;
			}
		}
	}
	
		pair<int,int> p; p.first = i; p.second = j;
		v.push_back(p);
	
	max=-32000;
	for(int q=0;q<v.size();q++){
		// cout << v[q].first << " " << v[q].second << endl;
		if(seq[v[q].second]-seq[v[q].first]>max){
			max = seq[v[q].second]-seq[v[q].first];
		}
	}
	cout << max << endl;


}