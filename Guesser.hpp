#pragma once

#include "IGuesser.hpp"
#include <bits/stdc++.h>
#include <pthread.h>

using Task = void* (*) (void*);

template<typename TaskArg>
struct ThreadPool {
    public:
    private:

        class ThreadSafeQueue
        {
            public:
            ThreadSafeQueue(): next() {
                pthread_mutex_init(&mut, NULL);
                pthread_cond_init(&con, NULL);
            }
            
            ~ThreadSafeQueue(){
                if(!isStopped) stop();
            }

            void push(void* arg) {
                pthread_mutex_lock(&mut);
                next.push(arg);
                pthread_mutex_unlock(&mut);
                pthread_cond_signal(&con);
            }

            void* pop(){
                pthread_mutex_lock(&mut);
                while(next.empty() && !isStopped) { pthread_cond_wait(&con, &mut); }
                if(isStopped) {
                    pthread_mutex_unlock(&mut);
                    return NULL;
                }

                void* val = next.front();
                next.pop();
    
                pthread_mutex_unlock(&mut);
                return val;
            }

            void* popIf(){
                pthread_mutex_lock(&mut);

                if(next.empty()){
                    pthread_mutex_unlock(&mut);
                    return NULL;
                }else{
                    void* val = next.front();
                    next.pop();
                    pthread_mutex_unlock(&mut);
                    return val;
                }
            }

            void stop(){
                isStopped = true;
                pthread_cond_broadcast(&con);
            }

            private:
                std::queue<void*> next;
                pthread_mutex_t mut;
                pthread_cond_t con;
                bool isStopped = false;
        };

        struct ThreadArg {
            Task task;
            void * arg;
            int* finished_count;
            pthread_cond_t* finished_cond;
            pthread_mutex_t* finished_mut;
        };

    public:
        
        const int thread_count;
        std::vector<pthread_t> threads;
        ThreadSafeQueue next;
        std::deque<ThreadArg> thread_args;
        std::deque<TaskArg> task_args;
        int finished_count = 0;
        pthread_cond_t finished_cond;
        pthread_mutex_t finished_mut;

        ThreadPool(const int thread_count)
            :thread_count(thread_count), threads(thread_count-1), next()
        {
            pthread_cond_init(&finished_cond, NULL);
            pthread_mutex_init(&finished_mut, NULL);
            for(int i = 0; i < thread_count - 1; i++){
                pthread_create(&threads[i], NULL, run, &next);
            }
        }

        ~ThreadPool(){
            next.stop();
            for(int i = 0; i < thread_count - 1; i++){
                pthread_join(threads[i], NULL);
            }
        }

        void wait(){
            ThreadArg* qarg;
            while((qarg = (ThreadArg*)next.popIf()) != NULL){
                qarg->task(qarg->arg);
                pthread_mutex_lock(&finished_mut);
                finished_count--;
                pthread_mutex_unlock(&finished_mut);

            }

            // could opt for a signal based solution instead of a splin lock
            pthread_mutex_lock(&finished_mut);
            while(finished_count != 0){
                pthread_cond_wait(&finished_cond, &finished_mut);
            }
            pthread_mutex_unlock(&finished_mut);


            thread_args.clear();
            task_args.clear();
        }

        void add(Task task, const TaskArg& arg){
            task_args.push_back(arg);
            thread_args.push_back({task, &task_args.back(), &finished_count, &finished_cond, &finished_mut});
            next.push(&thread_args.back());
            pthread_mutex_lock(&finished_mut);
            finished_count++;
            pthread_mutex_unlock(&finished_mut);

        }

    private:      

        Task run = [](void* args) ->void* {
            auto next = (ThreadSafeQueue*)(args);
            ThreadArg* qarg;
                
            while((qarg = (ThreadArg*)next->pop()) != NULL){
                qarg->task(qarg->arg);
                pthread_mutex_lock(qarg->finished_mut);
                (*qarg->finished_count)--;
                pthread_mutex_unlock(qarg->finished_mut);

                pthread_cond_signal(qarg->finished_cond);
            }
            return NULL;
        };

};

class Guesser : public IGuesser
{

    struct Word {
        char letters[5];
        int id;
        inline char& operator[](int k){ return letters[k]; }
        inline const char& operator[](int k) const{ return letters[k]; }
        inline const Word& operator=(const Word& rhs){
            memcpy(letters, rhs.letters, sizeof(char)*5);
            id = rhs.id;
            return *this;
        }
        inline std::string toString() const {
            std::string s = "";
            s += (char)(letters[0] + 'a');
            s += (char)(letters[1] + 'a');
            s += (char)(letters[2] + 'a');
            s += (char)(letters[3] + 'a');
            s += (char)(letters[4] + 'a');

            return s; 
        }

        inline bool operator==(const Word& rhs){
            return letters[0] == rhs.letters[0] &&
            letters[1] == rhs.letters[1] && letters[2] == rhs.letters[2] &&
            letters[3] == rhs.letters[3] && letters[4] == rhs.letters[4];
        }

        inline bool operator!=(const Word& rhs){
            return !(*this == rhs);
        }

        Word(){}

        // Word(const std::string& s){
        //     letters[0] = s[0] - 'a';
        //     letters[1] = s[1] - 'a';
        //     letters[2] = s[2] - 'a';
        //     letters[3] = s[3] - 'a';
        //     letters[4] = s[4] - 'a';
        // }
    };

    struct TaskArg {
        int from;
        int to;
        const int n;
        const std::vector<Word>& wordList;
        std::vector<float>& expected;
        std::vector<std::vector<int>>& codes;

    };

    struct PreCalcTaskArg {
        int from;
        int to;
        const int n;
        const std::vector<Word>& wordList;
        std::vector<std::vector<int>>& codes;
    };
    
    const Task precalcCodes = [](void* args) -> void* {
        auto&[from, to, n, wordList, codes] = *(PreCalcTaskArg*)(args);
        const int length = to - from + 1;

        std::array<int, 26> chars;
        std::array<int, 5> res;

        std::vector<int> ins(26*length, 0);
        for(int i = from; i <= to; i++){
            ins[(i-from)*26+wordList[i][0]]++;
            ins[(i-from)*26+wordList[i][1]]++;
            ins[(i-from)*26+wordList[i][2]]++;
            ins[(i-from)*26+wordList[i][3]]++;
            ins[(i-from)*26+wordList[i][4]]++;
        }

        std::vector<std::vector<int>> rem(3*81);        
        for(int i = from; i <= to; i++){
            for(int j = 0; j < n; j++){
                const auto& target = wordList[i];
                const auto& guess = wordList[j];
                const int di = (i-from)*26;

                chars.fill(0);

                chars[guess[0]] += !(res[0] = (guess[0] != target[0]));
                chars[guess[1]] += !(res[1] = (guess[1] != target[1]));
                chars[guess[2]] += !(res[2] = (guess[2] != target[2]));
                chars[guess[3]] += !(res[3] = (guess[3] != target[3]));
                chars[guess[4]] += !(res[4] = (guess[4] != target[4]));

            
                (res[0] != 0)&&(res[0] = ((++chars[guess[0]] > ins[di + guess[0]]) + 1));
                (res[1] != 0)&&(res[1] = ((++chars[guess[1]] > ins[di + guess[1]]) + 1));
                (res[2] != 0)&&(res[2] = ((++chars[guess[2]] > ins[di + guess[2]]) + 1));
                (res[3] != 0)&&(res[3] = ((++chars[guess[3]] > ins[di + guess[3]]) + 1));
                (res[4] != 0)&&(res[4] = ((++chars[guess[4]] > ins[di + guess[4]]) + 1));

                int code = res[0] + res[1]*3 + res[2]*9 + res[3]*27 + res[4]*81;
                codes[i][j] = code;
            }
        }

        return NULL;
    };

    const int thread_count = 4;
    ThreadPool<TaskArg> tp;
    std::vector<Word> arr;
    
    Word starter;
    Word lastGuess;
    std::vector<Word> remainder;
    std::vector<std::vector<int>> codes;

    const Task calcProb = [](void* args) -> void* {
        auto&[from, to, n, wordList, expected, codes] = *(TaskArg*)(args);
        
        const int length = to - from + 1;
        auto my_log = [](float x) -> float {return (x <= 0 ? 0 : std::log2(x));};

        std::vector<std::vector<float>> probs(length, std::vector<float>(3*81, 0));
        std::vector<std::vector<int>> rem(3*81);        
        for(int i = from; i <= to; i++){
            for(auto& row: rem) { row.clear(); }

            for(int j = 0; j < n; j++){
                const auto& target = wordList[i];
                const auto& guess = wordList[j];
                int code = codes[target.id][guess.id];
                probs[i-from][code]+=1;
                rem[code].push_back(wordList[j].id);
            }

            float mx = 0;
            std::sort(rem.begin(), rem.end(), [](const auto& a, const auto& b){return a.size() > b.size();});

            for(int j = 0; j < 3*81; j++){
                const int m = std::min(200, (int)rem[j].size());

                std::vector<std::vector<float>> probs_layer_2(m, std::vector<float>(3*81));
                for(int k = 0; k < m; k++){
                    for(int t = 0; t < m; t++){
                        const int& target = rem[j][k];
                        const int& guess = rem[j][t];
                        const int code = codes[target][guess];
                        probs_layer_2[k][code]++;
                    }
                }

                for(int k = 0; k < m; k++){
                    float expect = 0;
                    for(int t = 0; t < 3*81; t++){
                        const float p = probs_layer_2[k][t] / m;
                        if(p == 0) continue;
                        const float inf = my_log(1.0/p);
                        expect += p*inf;
                    }

                    if(expect < mx){
                        mx = expect;
                    }
                }

            }

            float expect = 0;
            for(int t = 0; t < 3*81; t++){
                const float p = probs[i-from][t] / n;
                if(p == 0) continue;
                const float inf = my_log(1.0/p);
                expect += p*inf;
            }
            expected[i] = expect + mx;
        }

        return NULL;
    };

public:
	Guesser (const std::vector <std::string>& fiveLetterWords)
        : IGuesser (fiveLetterWords), tp(thread_count), arr(fiveLetterWords.size())
	{
        const int n = fiveLetterWords.size();
        std::vector<int> indexes(n);
        std::iota(indexes.begin(), indexes.end(), 0);
        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(indexes.begin(), indexes.end(), g);
        codes.resize(n, std::vector<int>(n, 0));

        for(int i = 0; i < n; i++) {
            const int index = indexes[i];
            arr[index][0] = fiveLetterWords[i][0]-'a';
            arr[index][1] = fiveLetterWords[i][1]-'a';
            arr[index][2] = fiveLetterWords[i][2]-'a';
            arr[index][3] = fiveLetterWords[i][3]-'a';
            arr[index][4] = fiveLetterWords[i][4]-'a';
            arr[index].id = index;
        }
        
        {

            ThreadPool<PreCalcTaskArg> pre(thread_count);
            for(int i = 0; i < thread_count; i++){
                pre.add(precalcCodes, PreCalcTaskArg{i*n/thread_count, (i+1)*n/thread_count-1, n, std::ref(arr), std::ref(codes)});
            }
            pre.wait();
        }

        lastGuess = remOptimumL1(arr);
        remainder = arr;

        starter = lastGuess;
    }

    Word remOptimumL1(std::vector<Word>& wordList){
        const int n = wordList.size();
        std::vector<float> expected(n, 0);

        if(wordList.size() > 24){
            for(int i = 0; i < thread_count; i++){
                tp.add(calcProb, TaskArg{i*n/thread_count, (i+1)*n/thread_count-1, n, std::ref(wordList), std::ref(expected), std::ref(codes)});
            }
            tp.wait();
        }else{
            TaskArg arg{0, n-1, n, std::ref(wordList), std::ref(expected), std::ref(codes)};
            calcProb(&arg);
        }

        int mxi = 0;
        for(int i = 1; i < n; i++){
            if(expected[i] > expected[mxi]){
                mxi = i;
            }
        }

        Word ans = wordList[mxi];
        wordList[mxi] = wordList.back();
        wordList.pop_back();

        return ans;
    }

	virtual std::string MakeAGuess(std::vector<std::vector<LetterResult>> results) override
	{
        if(results.empty()) { return starter.toString(); }
        
        std::vector<LetterResult>& tips = results.back();
        const int devc = tips[0]*1 + tips[1]*3 + tips[2]*9 + tips[3]*27 + tips[4]*81;

        remainder.resize( std::partition(remainder.begin(), remainder.end(), [&](const Word& target){
            return (codes[target.id][lastGuess.id] == devc);
        }) - remainder.begin());

        if(remainder.size() == 1){ return remainder[0].toString(); }

        lastGuess = remOptimumL1(remainder);
        return lastGuess.toString();
	}

	virtual void Reset () override
	{
        remainder = arr;
        lastGuess = starter;
	}
};
