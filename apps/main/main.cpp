/**
 * @file main.cpp
 * @author karurochari
 * @brief 
 * @version 0.1
 * @date 2020-04-20
 * 
 * @copyright Copyright (c) 2020
 * 
 */


#include <iostream>
#include <filesystem>
#include <unistd.h>


#include "simulator_t.h"
#include "workers-queue.h"
#include "basic-callback.h"

using namespace std;

#include <nlohmann/json.hpp>

using namespace nlohmann;

struct fake_model{
    struct state_t{
        friend void to_json(json& i, const state_t& m){}
        friend void from_json(const json& j, state_t& m){}

        state_t(){}

        state_t& operator+=(const state_t& a){return *this;}
    };

    struct mstate_t{
        friend void to_json(json& i, const mstate_t& m){}
        friend void from_json(const json& j, mstate_t& m){}

        mstate_t(){}
    };

    typedef state_t delta_state_t;
    //struct delta_state_t{};

    struct termination_t{
        friend void to_json(json& i, const termination_t& m){}
        friend void from_json(const json& j, termination_t& m){}

        bool operator()(const state_t& s) const{return (rand()%100==1);}
    };

    friend void to_json(json& i, const fake_model& m){}
    friend void from_json(const json& j, fake_model& m){}

    inline const static bool differential=true;
    inline const static bool recoverable=true;

    template<typename T>
    delta_state_t operator()(const state_t& a, mstate_t& b, const T& env) const {usleep(5000);return delta_state_t();}
};

struct fake_callback{
    friend void to_json(json& i, const fake_callback& m){}
    friend void from_json(const json& j, fake_callback& m){}

    template<typename T>
    void operator()(const T& i) const{}
};

struct fake_tweaks{
    friend void to_json(json& i, const fake_tweaks& m){}
    friend void from_json(const json& j, fake_tweaks& m){}
};


int main(int argc, const char* argv[]){
    std::string initial_data;
    for(;std::cin;){
        std::string tmp;
        std::getline(std::cin,tmp);
        initial_data+=tmp;
    }

    //nlohmann::json config="{\"out-dir\":\"/tmp/h3\", \"model\":{}}"_json;
    nlohmann::json config=nlohmann::json::parse(initial_data);

    if(argc>=2 && std::string(argv[1])=="continue")config["continue"]=true;

    try{
        simulator_t<fake_model,basic_callback,fake_tweaks> sim(config);
        std::cout<<"######################################################\n\n";
        sim();
    }
    catch(std::exception& e){
        std::cerr<<e.what()<<"\n";
        return 1;
    }

    std::cout<<"######################################################\n\n";
    std::cout<<"お早うセカイ\n";
    return 0;
}
