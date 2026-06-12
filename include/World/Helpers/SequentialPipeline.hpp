
#pragma once


#include <functional>


using std::function;

class SequentialPipeline
{
    struct FunctionNode
    {
        function<bool()> func;     
    };

    public:
        SequentialPipeline& next(function<bool()> action)
        {
            pipeline.push_back({action});
            return *this;
        }

        void onUpdate(float totalTime)
        {
            if(pipeline.empty())
                return;

            if(pipeline.front().func())
            {
                pipeline.erase(pipeline.begin());
            }
        }

    private:
        std::vector<FunctionNode> pipeline;
};