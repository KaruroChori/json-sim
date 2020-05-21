#pragma once

/**
 * @file workers-queue.h
 * @author karurochari
 * @brief 
 * @version 0.1
 * @date 2020-04-20
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#include <iostream>
#include <functional>
#include <stdexcept>
#include <queue>
#include <map>
#include <thread>

#include "string-exception.h"

template <typename T>
struct workers_queue;


template <typename T>
struct workers_queue{
    private:
        struct thread_t;

        uint                        max_queue;          ///< The maximum number of concurrent threads to be excuted at any time.
        uint                        active_n=0;         ///< The number of active tasks.
        uint                        next_id=0;          ///< The next id to be used.
        std::queue<uint>            remove_next;        ///< The id of the thread to be disabled.
        std::map<uint,thread_t*>    threads;            ///< The container for all threads, both running and not.

        std::condition_variable     cv;
        std::mutex                  m;

    public:
        workers_queue(uint l=1):max_queue(l){}

        ~workers_queue(){for(auto [i,j]:threads)delete j;}

        /**
         * @brief Start executing all the tasks on the queue, with a maximum at any time fixed.
         * 
         * @param cc a reference to the generator of tasks to be iterated over.
         * @param keep_track should the library keep some information on the completed task to be used later on?
         * @param verbose should the function print a final report on the tasks performed?
         * @param out
         * @param err
         * @return int 0 if everything went ok, else any other value.
         */
        int operator()(const T& cc, bool keep_track=true, bool verbose=true, std::ostream& out=std::cout, std::ostream& err=std::cerr){
            uint bad_counter=0;

            for(auto ii=cc.begin();ii!=cc.end() or active_n!=0;){

                std::unique_lock<std::mutex> lock(m);
                for(;active_n<max_queue && ii!=cc.end();++ii){
                    auto& th=*(threads[next_id++]=new thread_t(next_id));
                    th.exec()=std::move(*ii);
                    
                    active_n++;
                    if(verbose)out<<"Started   ["<<th.id()<<"]\n";

                    th.thread()=std::thread([&](thread_t* _myself){
                        thread_t& myself=*_myself;
                        try{
                            auto start = std::chrono::system_clock::now();
                            myself.ret_val()=myself.exec()();
                            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start);
                            myself.duration()=duration.count();

                        }
                        /*catch(const std::exception& ex){
                            myself.exception()=true;
                            myself.exception_handle()=ex;
                        }*/
                        catch(...){
                            myself.exception()=true;
                        }

                        {
                            std::unique_lock<std::mutex> lock(m);
                            remove_next.push(myself.id());
                            lock.unlock();
                            cv.notify_all();
                        }

                    },&th);
                    th.thread().detach();
                }

                cv.wait(lock, [&](){return remove_next.size()!=0;});

                for(;remove_next.size()!=0;){
                    const auto delendo=threads.find(remove_next.front());
                    const thread_t& th=*(delendo->second);

                    if(verbose){
                        if(th.exception()){
                            /*if(th.exception_handle().has_value())err<<"Exception in task ["<<th.id()<<"]: "<<th.exception_handle().value().what()<<"\n";
                            else */err<<"Exception in task ["<<th.id()<<"]\n";
                        }
                        else out<<"Completed ["<<th.id()<<"]\tin "<<th.duration()<<". Returned ["<<th.ret_val()<<"]\n";
                    }
                    
                    if(th.exception() or th.ret_val()!=0)bad_counter++;

                    if(!keep_track){
                        auto it=threads.find(th.id());
                        delete it->second;
                        threads.erase(it);
                    }

                    remove_next.pop();
                    active_n--;
                }

                lock.unlock();
                cv.notify_all();
            }

            if(verbose && bad_counter!=0){
                out<<"Queue completed. ["<<bad_counter<<"] tasks failed.";
            }

            return bad_counter;
        }

    private:

        struct thread_t{
            public:
                thread_t(uint i):_id(i){}

                inline uint id() const{return _id;}

                inline int ret_val() const{return _ret_val;}
                inline int& ret_val() {return _ret_val;}

                inline uint duration() const{return _duration;}
                inline uint& duration() {return _duration;}

                inline bool exception() const{return _exception;}
                inline bool& exception() {return _exception;}

                inline const std::thread& thread() const{return _thread;}
                inline std::thread& thread() {return _thread;}

                inline const std::function<int()>& exec() const{return _exec;}
                inline std::function<int()>& exec() {return _exec;}

            private:
                uint                            _id;
                int                             _ret_val;
                uint                            _duration;
                bool                            _exception=false;
                std::thread                     _thread;
                std::function<int()>            _exec;
        };
};