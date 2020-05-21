#pragma once

/**
 * @file simulator_t.h
 * @author karurochari
 * @brief 
 * @version 0.1
 * @date 2020-04-20
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#include <iostream>
#include <string>
#include <filesystem>
#include <fstream>


//Source location is not fully supported, come back later.
//#include <source_location>


#include <nlohmann/json.hpp>

#include "string-exception.h"
#include "workers-queue.h"

//Concepts. At the moment they are not fully supported, come back later.
template<typename T>
concept ModelType = requires(){
    true;
};

template<typename T>
concept CallbackType = requires(){
    true;
};

template<typename T>
concept TweaksType = requires(){
    true;
};

/**
 * @brief Simulator interface in a single class.
 * Generate the instance of a simulator. Its usage is extremely simple:
 * ```
 * simulator_t<model_v,callback_v,tweaks_v> simul;
 * simul();
 * ```
 * That is it. This class was mainly written to deal with the usage of arbitrary user-implemented models, callbacks and tweaks.
 * @tparam MODEL_T the type of model used.
 * @tparam CALLBACK_T the type of callbacks used.
 * @tparam TWEAKS_T the custom tweak structure which is highly application dependent.
 */
template<ModelType MODEL_T, CallbackType CALLBACK_T, TweaksType TWEAKS_T>
struct simulator_t;

template<ModelType MODEL_T, CallbackType CALLBACK_T, TweaksType TWEAKS_T>
struct simulator_t{
    public:
    
        struct task_batch_t;
        struct task_t;
        struct const_iterator;

        friend task_batch_t;
        friend task_t;
        friend const_iterator;

        typedef MODEL_T     model_t;
        typedef CALLBACK_T  callback_t;
        typedef TWEAKS_T    tweaks_t;

        /**
         * @brief Simulation time!
         * @param _in the default input stream
         * @param _out the default output stream
         * @param _err the default error stream
         */
        simulator_t(const nlohmann::json& data, std::ostream& _out=std::cout, std::ostream& _err=std::cerr);

        /**
         * @brief Start the simulation
         * @returns 0 if all the tasks were properly completed, any other number if there has been something wrong.
         */
        int operator()();

        struct task_batch_t{
            friend simulator_t;
            friend task_t;
            task_batch_t(const simulator_t& p, const std::string& name, const nlohmann::json& data);
            task_batch_t(task_batch_t&& c)=default;

            private:
                std::string                     name;
                typename model_t::state_t       initial_state;          ///< The initial state for all the tasks in this batch.
                std::optional<callback_t>       batch_callback={};      ///< The optional callback for when the batch is completed.
                std::optional<callback_t>       instance_callback={};   ///< The optional callback for when the single instance is completed.
                std::optional<callback_t>       event_callback={};      ///< The optional callback for each simulation step.
                std::optional<tweaks_t>         tweaks={};              ///< Optional directives which are application specific.
                typename model_t::termination_t end_condition;          ///< The conditions which should not be met to continue the simulation.
                uint                            instances=1;            ///< The number of tasks to be spawaned with this same initial configuration.
                uint                            sync=0;                 ///< How many simulation steps I have to skip way before synchronizing with my storage.
                uint                            backup=0;               ///< How many synchronization steps I have to skip before updateing the backup copy.
                bool                            save_trace=true;        ///< Should the trace be saved or only the final state?
                bool                            save_mstate=false;      ///< Should I save the model state?

                const simulator_t&              parent;                 ///< A reference to the parent simulation.
        };

        struct task_t{
            typedef std::vector<typename model_t::delta_state_t> trajectory_t;
            task_t(const task_batch_t& p, uint _id);
            int operator()();

            private:
                uint                            id;
                typename model_t::state_t       current_state;          ///< The current state of the simulation instance.
                trajectory_t                    trajectory;             ///< All the events up to this point which have not been copied on disk yet. If save_trace is set to false it is empty.
                typename model_t::mstate_t      model_state;            ///< The expanded variables for the model state as it is evolving as well.                                                 

                const task_batch_t&             parent;                 ///< A reference to the task pool this instance is part of.

        };

        struct const_iterator{
            private:
                friend simulator_t;

                const std::map<std::string,task_batch_t>& r;
                typename std::map<std::string,task_batch_t>::const_iterator it;
                uint residual=0;

                const_iterator(const std::map<std::string,task_batch_t>& ref):r(ref){}

            public:
                std::string where(){return it->first+"/"+std::to_string(residual);}
                std::function<int()> operator*(){
                    //If not my iterator will have changed by the time I am using it in the lambda.
                    auto cpit=*this;
                    return std::function<int()>([cpit]()->int{
                        task_t tmp(cpit.it->second,cpit.residual);return tmp();
                    });
                }
                friend bool operator!=(const const_iterator& a, const const_iterator& b){return (a.residual!=b.residual) or (a.it!=b.it);}
                
                const_iterator& operator++(){
                    for(;residual==0 && it!=r.end();){
                        it++;
                        if(it!=r.end())residual=it->second.instances;
                        else residual=0;
                    }

                    if(it!=r.end()){
                        residual--;
                    }
                    return *this;
                }
        };


        inline const_iterator begin() const{const_iterator ret(task_batches);ret.it=task_batches.begin();ret.residual=ret.it->second.instances;++ret;return ret;}
        inline const_iterator end() const{const_iterator ret(task_batches);ret.it=task_batches.end();ret.residual=0;return ret;}

    private:
        std::ostream&                       out;
        std::ostream&                       err;

        bool                                continue_mode;      ///< Is the task continueing from a previous stage? Or is is from scratch?
        std::string                         workspace;          ///< The directory where to work.
        model_t                             model;              ///< The specific model for this instance.
        uint                                parallel_max;       ///< How many workers I can have at any point in time.
        std::optional<callback_t>           global_callback={}; ///< The callback to be used before ending the process.
        std::optional<tweaks_t>             tweaks={};          ///< The application specific parameters.
        std::string                         license;            ///< The adopted license. By default it is considered as not permissive and commercial.

        std::map<std::string,task_batch_t>  task_batches;       ///< The batches of tasks to be executed.

        bool                                throw_wrong_type=false;
        bool                                verbose_messages=false;

        //TODO: Not yet configurable!
        uint                                default_instances=1;
        uint                                default_sync=0;
        uint                                default_backup=0;
        bool                                default_save_trace=true;
        bool                                default_save_mstate=false;

        /**
         * @brief Helper function to process a type matching error
         * 
         * @param field 
         * @param excepted 
         * @param optional 
         */
        void _type_mismatch(const std::string& field, const std::string& expected, bool optional) const{
            if(throw_wrong_type or !optional)err<<"Error: ";
            else err<<"Warning: ";
            err<<"the field ["<<field<<"] is defined but its type was expected to be ["<<expected<<"]. ";
            if(throw_wrong_type or !optional){err<<"An exception will be thrown.\n";throw StringException("TypeMismatchException");}
            else err<<"The default value will be used and this directive is going to be skipped.\n";
        }

        void _missing_field(const std::string& field) const{
            err<<"Error: the field ["<<field<<"] is missing and is required. An exception will be thrown.\n";
            throw StringException("MissingFieldException");
        }
};

template<ModelType M, CallbackType C, TweaksType T>
simulator_t<M,C,T>::simulator_t(const nlohmann::json& config, std::ostream& _out, std::ostream& _err):out(_out),err(_err){
    //Look for a *continue* directive
    {
        auto it=config.find("continue");
        if(it!=config.end() && it->is_boolean()){
            continue_mode=*it;
        }
        else if(it!=config.end()){
            _type_mismatch("continue","boolean",true);
        }
        else continue_mode=false;
    }

    //Set up the workspace directory
    {
        auto it=config.find("workspace");
        if(it!=config.end() && it->is_string()){
            workspace=*it;

            //I need to create a directory
            if(!continue_mode){
                out<<"Creating a new workspace in ["<<workspace<<"]\n";
                if(std::filesystem::create_directories(workspace)){}
                else{
                    err<<"Error: Unable to create directories ["<<(workspace)<<"]. An exception will be thrown.\n";
                    throw StringException("DirectoryCreationException");
                }
            }
            //I assume everything is already there.
            else{
                out<<"Resuming the workspace in ["<<workspace<<"]\n";
            }
        }
        else if(it!=config.end())_type_mismatch("workspace","string",false);
        else _missing_field("workspace");
    }

    //Load and patch the model!
    {
        auto it=config.find("model");
        if(it!=config.end()){
            nlohmann::json model_json=*it;
            //Patches for the model. Mind his kicks, you do not want to fall from the edge of Anor Londo.
            {
                auto it_2=config.find("patches");
                if(it_2!=config.end() && it_2->is_array()){
                    for(auto& i:*it_2){
                        try{
                            model_json.merge_patch(i);
                        }
                        catch(std::exception& e){
                            err<<"Error: "<<e.what()<<". Patch failed. An exception will be thrown.\n";
                            throw "UnrecognizedPatchException";
                        }
                    }
                    out<<"Model patched successfully!\n";
                }
                else if(it_2!=config.end()){
                    _type_mismatch("patches","array",true);
                }
                else{}  //No patches. Ok!
            }

            //Parse the model
            try{
                from_json(model_json, model);
            }
            catch(std::exception& e){
                err<<"Error: "<<e.what()<<". The model is not compatible. An exception will be thrown.\n";
                throw StringException("UnrecognizedModelException");
            }

            out<<"Model loaded.\n";

            //Save the patched model to be referred later.

        }
        else _missing_field("model");
    }

    //Detect the global callback
    {
        auto it=config.find("callback");
        if(it!=config.end()){
            global_callback=callback_t();
            try{
                from_json(*it, global_callback.value());
            }
            catch(std::exception& e){
                err<<"Error: "<<e.what()<<". The callback is not compatible. An exception will be thrown.\n";
                throw StringException("UnrecognizedCallbackException");
            }
        }
        else global_callback={};
    }

    //The custom & optional tweaks field.
    {
        auto it=config.find("tweaks");
        if(it!=config.end()){
            tweaks=tweaks_t();
            try{
                from_json(*it, tweaks.value());
            }
            catch(std::exception& e){
                err<<"Error: "<<e.what()<<". The tweaks are not compatible. An exception will be thrown.\n";
                throw StringException("UnrecognizedTweaksException");
            }
        }
        else tweaks={};
    }

    //Tasks!
    {
        auto it=config.find("tasks");
        if(it!=config.end() && it->is_object()){
            //Register the task batches!
            for(auto& i:it->items()){
                try{
                    task_batches.emplace(i.key(),task_batch_t(*this,i.key(),i.value()));
                }
                catch(std::exception& e){
                    err<<"Error: "<<e.what()<<". The structure of task ["<<i.key()<<"] is not compatible. An exception will be thrown.\n";
                    throw StringException("MisformedTaskException");
                }
            }
        }
        else if(it!=config.end())_type_mismatch("tasks","map",false);
        else _missing_field("tasks");
    }

    //The custom & optional tweaks field.
    {
        auto it=config.find("parallel");
        if(it!=config.end() && it->is_number_unsigned()){
            parallel_max=*it;
        }
        else if(it!=config.end()){
            _type_mismatch("parallel","unsigned integer",true);
        }
        else parallel_max=std::thread::hardware_concurrency();
    }

    out<<"Configuration completed, ready to run!\n";

}

template<ModelType M, CallbackType C, TweaksType T>
simulator_t<M,C,T>::task_batch_t::task_batch_t(const simulator_t& p, const std::string& _name, const nlohmann::json& config):name(_name),parent(p),initial_state(){
    //End-state! It **MUST** be defined.
    {
        auto it=config.find("end-condition");
        if(it!=config.end() && it->is_object()){
            try{
                from_json(*it,end_condition);
            }
            catch(std::exception& e){
                p.err<<"Error: "<<e.what()<<". The structure of end-condition is not compatible. An exception will be thrown.\n";
                throw StringException("MisformedEndConditionException");
            }
        }
        else if(it!=config.end())p._type_mismatch("end-condition","object",false);
        else p._missing_field("end-condition");
    }

    //Initial state. By default set to "0".
    {
        auto it=config.find("initial-state");
        if(it!=config.end() && it->is_object()){
            try{
                from_json(*it,initial_state);
            }
            catch(std::exception& e){
                p.err<<"Error: "<<e.what()<<". The structure of initial-state is not compatible. An exception will be thrown.\n";
                throw StringException("MisformedEndConditionException");
            }
        }
        else if(it!=config.end())p._type_mismatch("initial-state","object",true);
        else;
    }

    //Instances. 1 by default.
    {
        auto it=config.find("instances");
        if(it!=config.end() && it->is_number_unsigned()){
            instances=*it;
        }
        else if(it!=config.end()){
            p._type_mismatch("instances","unsigned integer",true);
        }
        else instances=p.default_instances;
    }

    //Sync. 0 by default.
    {
        auto it=config.find("sync");
        if(it!=config.end() && it->is_number_unsigned()){
            sync=*it;
        }
        else if(it!=config.end()){
            p._type_mismatch("sync","unsigned integer",true);
        }
        else sync=p.default_sync;
    }

    //Backup. 0 by default.
    {
        auto it=config.find("backup");
        if(it!=config.end() && it->is_number_unsigned()){
            backup=*it;
        }
        else if(it!=config.end()){
            p._type_mismatch("backup","unsigned integer",true);
        }
        else backup=p.default_backup;
    }

    //Save trace. true by default.
    {
        auto it=config.find("save-trace");
        if(it!=config.end() && it->is_boolean()){
            save_trace=*it;
        }
        else if(it!=config.end()){
            p._type_mismatch("save-trace","boolean",true);
        }
        else save_trace=p.default_save_trace;
    }

    //Save the model state. Please keep in mind that this operation may not be supported. false by default.
    {
        auto it=config.find("save-model-state");
        if(it!=config.end() && it->is_boolean()){
            save_mstate=*it;
        }
        else if(it!=config.end()){
            p._type_mismatch("save-model-state","boolean",true);
        }
        else save_mstate=p.default_save_mstate;
    }

    //Detect the global callback
    {
        auto it=config.find("batch-callback");
        if(it!=config.end()){
            batch_callback=callback_t();
            try{
                from_json(*it, batch_callback.value());
            }
            catch(std::exception& e){
                p.err<<"Error: "<<e.what()<<". The batch callback is not compatible. An exception will be thrown.\n";
                throw StringException("UnrecognizedCallbackException");
            }
        }
        else batch_callback={};
    }

    //Detect the instance callback
    {
        auto it=config.find("callback");
        if(it!=config.end()){
            instance_callback=callback_t();
            try{
                from_json(*it, instance_callback.value());
            }
            catch(std::exception& e){
                p.err<<"Error: "<<e.what()<<". The instance callback is not compatible. An exception will be thrown.\n";
                throw StringException("UnrecognizedCallbackException");
            }
        }
        else instance_callback={};
    }

    //Detect the event callback
    {
        auto it=config.find("batch-callback");
        if(it!=config.end()){
            event_callback=callback_t();
            try{
                from_json(*it, event_callback.value());
            }
            catch(std::exception& e){
                p.err<<"Error: "<<e.what()<<". The event callback is not compatible. An exception will be thrown.\n";
                throw StringException("UnrecognizedCallbackException");
            }
        }
        else event_callback={};
    }

    //The custom & optional tweaks field.
    {
        auto it=config.find("tweaks");
        if(it!=config.end()){
            tweaks=tweaks_t();
            try{
                from_json(*it, tweaks.value());
            }
            catch(std::exception& e){
                p.err<<"Error: "<<e.what()<<". The tweaks are not compatible. An exception will be thrown.\n";
                throw StringException("UnrecognizedTweaksException");
            }
        }
        else tweaks={};
    }
}

template<ModelType M, CallbackType C, TweaksType T>
simulator_t<M,C,T>::task_t::task_t(const simulator_t<M,C,T>::task_batch_t& p, uint _id):parent(p),id(_id){}

template<ModelType M, CallbackType C, TweaksType T>
int simulator_t<M,C,T>::operator()(){
    workers_queue<simulator_t> queue(parallel_max);
    queue(*this,true,true,out,err);
    if(global_callback.has_value())global_callback.value()(*this);
    return 0;
}

template<ModelType M, CallbackType C, TweaksType T>
int simulator_t<M,C,T>::task_t::operator()(){
    std::string task_name=parent.name+"/"+std::to_string(id);
    std::string dir=parent.parent.workspace+"/tasks/"+task_name;
    std::filesystem::create_directories(dir);
    std::ofstream out(dir+"/.out", std::ios_base::app);
    std::ofstream err(dir+"/.err", std::ios_base::app);

    if(!out){parent.parent.err<<"Unable to open the [out] stream for task ["+task_name+"]\n";throw StringException("OutStreamFailure");}
    if(!err){parent.parent.err<<"Unable to open the [err] stream for task ["+task_name+"]\n";throw StringException("ErrStreamFailure");}

    if(parent.parent.continue_mode){
        std::string task_name=parent.name+"/"+std::to_string(id);
        std::string dir=parent.parent.workspace+"/tasks/"+task_name;
        try{
            //Recover the file from the backup in folder.
            {
                std::ifstream state(dir+"/status.copy");
                nlohmann::json tmp;
                state>>tmp;
                from_json(tmp,current_state);
                state.close();
            }

            //If the mstate is set as recoverable recover it as well.
            if constexpr (model_t::recoverable){
                if(parent.save_mstate){
                    std::ifstream state(dir+"/mstatus.copy");
                    nlohmann::json tmp;
                    state>>tmp;
                    from_json(tmp,model_state);
                    state.close();
                }
            }
        }
        catch(...){
            err<<"Unable to properly process the initial state. The default one will be applied.\n";
            current_state=parent.initial_state;
            //model_state; Not yet decided what to do about this :). @TODO
        }
    }
    else{
        current_state=parent.initial_state;
        //model_state; Not yet decided what to do about this :). @TODO
    }

    try{

        for(uint step=0;!parent.end_condition(current_state);step++){
            if(step!=0 && (step%((parent.sync+1)*(parent.backup+1)))==0){
                //Backup stuff
                std::filesystem::copy(dir+"/status",dir+"/status.copy",std::filesystem::copy_options::overwrite_existing);
                if(parent.save_mstate)std::filesystem::copy(dir+"/mstatus",dir+"/mstatus.copy",std::filesystem::copy_options::overwrite_existing);
                if(parent.save_trace){
                    std::ofstream trace(dir+"/trace.copy",std::ios_base::app);
                    for(uint i=(parent.sync+1)*(parent.backup+1);i!=0;i--){
                        nlohmann::json tmp;
                        to_json(tmp,trajectory[trajectory.size()-i]);
                        trace<<tmp<<(char)31;   //Divide the unit of a record.
                    }
                    trajectory.clear();
                    trace.close();
                }
            }
            if((step%(parent.sync+1))==0){
                //Save the state
                {
                    std::ofstream status(dir+"/status");
                    nlohmann::json tmp;
                    to_json(tmp,current_state);
                    status<<tmp;
                    status.close();
                }
                if(parent.save_mstate){
                    std::ofstream mstatus(dir+"/mstatus");
                    nlohmann::json tmp;
                    to_json(tmp,model_state);
                    mstatus<<tmp;
                    mstatus.close();
                }
                if(step!=0 && parent.save_trace){
                    std::ofstream trace(dir+"/trace",std::ios_base::app);
                    for(uint i=parent.sync+1;i!=0;i--){
                        nlohmann::json tmp;
                        to_json(tmp,trajectory[trajectory.size()-i]);
                        trace<<tmp<<(char)31;   //Divide the unit of a record.
                    }
                    trace.close();
                }
            }

            if constexpr(M::differential){
                typename M::delta_state_t tmp=parent.parent.model(current_state,model_state,*this);
                current_state+=tmp;

                if(parent.save_trace)trajectory.push_back(tmp);
            }
            else{
                if(parent.save_trace){
                    auto old=current_state;
                    current_state=parent.parent.model(current_state,model_state,*this);
                    trajectory.push_back(current_state-old);
                }
                else current_state=parent.parent.model(current_state,model_state,*this);
            }

            if(parent.event_callback.has_value())parent.event_callback.value()(*this);
        }
    }
    catch(std::exception& e){
        err<<"Exception triggered: "<<e.what()<<"\n";
        return 1;
    }

    //Execute the final save task.
    {
        //Save all
        {
            std::ofstream status(dir+"/status");
            nlohmann::json tmp;
            to_json(tmp,current_state);
            status<<tmp;
            status.close();
        }
        if(parent.save_mstate){
            std::ofstream mstatus(dir+"/mstatus");
            nlohmann::json tmp;
            to_json(tmp,model_state);
            mstatus<<tmp;
            mstatus.close();
        }
        if(parent.save_trace){
            std::ofstream trace(dir+"/trace",std::ios_base::app);
            for(auto& t:trajectory){
                nlohmann::json tmp;
                to_json(tmp,t);
                trace<<tmp<<(char)31;   //Divide the unit of a record.
            }
            trace.close();
        }

        //Backup the last copies for restart.
        std::filesystem::copy(dir+"/status",dir+"/status.copy",std::filesystem::copy_options::overwrite_existing);
        if(parent.save_mstate)std::filesystem::copy(dir+"/mstatus",dir+"/mstatus.copy",std::filesystem::copy_options::overwrite_existing);
        if(parent.save_trace){
            std::ofstream trace(dir+"/trace.copy",std::ios_base::app);
            for(auto& t:trajectory){
                nlohmann::json tmp;
                to_json(tmp,t);
                trace<<tmp<<(char)31;   //Divide the unit of a record.
            }
            trajectory.clear();
            trace.close();
        }
    }

    if(parent.instance_callback.has_value())parent.instance_callback.value()(*this);
    if(id==0){
        if(parent.batch_callback.has_value())parent.batch_callback.value()(parent);
    }
    return 0;
}