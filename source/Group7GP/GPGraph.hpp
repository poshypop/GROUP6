
#pragma once

#include <cassert>
#include <map>
#include <string>
#include <algorithm>
#include <iostream>
#include <vector>

namespace cowboys
{
    class GraphNode
    {
    protected:
        std::vector<std::shared_ptr<GraphNode>> inputs;
        std::vector<size_t> inputs_indices;

        /// The output of this node.
        double output = 0.;

    public:
        GraphNode() = default;
        virtual ~GraphNode() = default;

        /// TODO: Check guidelines for this
        GraphNode(double default_value) : output{default_value} {}

        /// @brief Constructor for input indices
        /// @param input_indices
        GraphNode(std::vector<size_t> input_indices) : inputs_indices{input_indices} {}

        std::string name;

        /// TODO: Cache outputs
        virtual double GetOutput() const { return output; }

        std::shared_ptr<GraphNode> GetInput(size_t input_idx) const
        {
            if (input_idx >= inputs.size())
                // Return last input if out of bounds
                return inputs.back();
            else
                return inputs.at(input_idx);
        }

        void AddInput(std::shared_ptr<GraphNode> node) { inputs.push_back(node); }
        void AddInputs(const std::vector<std::shared_ptr<GraphNode>> &nodes) { inputs.insert(inputs.end(), nodes.begin(), nodes.end()); }
        void SetInputs(const std::vector<std::shared_ptr<GraphNode>> &nodes) { inputs = nodes; }
        void SetOutput(double value) { output = value; }
    };

    class GateNode : public GraphNode
    {
    public:
        GateNode() = default;
        virtual ~GateNode() = default;

        double GetOutput() const override
        {
            // input 0 is the input
            // input 1 is the condition
            if (inputs.size() < 2)
                return 0.;

            // Let the signal through if the condition is not 0
            return GetInput(1)->GetOutput() != 0. ? GetInput(0)->GetOutput() : 0.;
        }
    };

    class AndNode : public GraphNode
    {
    public:
        AndNode() = default;
        virtual ~AndNode() = default;

        double GetOutput() const override
        {
            // input 0 and 1 are the inputs
            if (inputs.size() < 2)
                return 0.;
            auto condition = GetInput(0)->GetOutput() != 0. && GetInput(1)->GetOutput() != 0.;
            return condition ? 1. : 0.;
        }
    };

    class AnyEqNode : public GraphNode
    {
    public:
        AnyEqNode() = default;
        virtual ~AnyEqNode() = default;

        double GetOutput() const override
        {
            // input 0 is the input, the rest is the values that we are checking against
            if (inputs.size() < 2)
                return 0.;
            for (size_t i = 1; i < inputs.size(); ++i)
            {
                auto condition = GetInput(0)->GetOutput() == GetInput(i)->GetOutput();
                if (condition)
                    return 1.;
            }
            return 0.;
        }
    };

    class NotNode : public GraphNode
    {
    public:
        NotNode() = default;
        virtual ~NotNode() = default;

        double GetOutput() const override
        {
            // input 0 is the value to not
            if (inputs.size() < 1)
                return 0.;
            auto condition = GetInput(0)->GetOutput() == 0.;
            return condition ? 1. : 0.;
        }
    };

    class SumNode : public GraphNode
    {
    public:
        SumNode() = default;
        virtual ~SumNode() = default;

        double GetOutput() const override
        {
            // Sum all inputs
            double sum = 0.;
            for (auto &input : inputs)
            {
                sum += input->GetOutput();
            }
            return sum;
        }
    };

    class Graph
    {
    protected:
        /// Actions that can be taken.
        std::vector<size_t> actions;

        /// Layers of nodes in the graph.
        std::vector<std::vector<std::shared_ptr<GraphNode>>> layers;

    public:
        Graph(const std::vector<size_t> &action_vec) : actions{action_vec} { assert(actions.size() > 0); }
        ~Graph() = default;

        size_t MakeDecision(const std::vector<double> &inputs)
        {
            if (layers.size() == 0)
                return actions.at(0);

            // Set inputs of input layer
            size_t i = 0;
            for (auto &node : layers[0])
            {
                double input = 0;
                if (i < inputs.size())
                    input = inputs.at(i);
                node->SetOutput(input);
                ++i;
            }

            // Get output of last layer
            std::vector<double> outputs;
            for (auto &node : layers.back())
            {
                outputs.push_back(node->GetOutput());
            }

            std::cout << "Outputs: ";
            for (auto &output : outputs)
            {
                std::cout << output << " ";
            }
            std::cout << std::endl;

            // Choose the action with the highest output
            auto max_output = std::max_element(outputs.begin(), outputs.end());
            size_t index = std::distance(outputs.begin(), max_output);

            // If index is out of bounds, return the last action
            size_t action = 0;
            if (index >= actions.size())
                action = actions.back();
            else
                action = actions.at(index);
            std::cout << "Action: " << action << std::endl;
            return action;
        }

        void AddLayer(const std::vector<std::shared_ptr<GraphNode>> &layer) { layers.push_back(layer); }
    };

    class GraphBuilder
    {
    protected:
        /// Actions that can be taken.
        std::vector<size_t> action_vec;

    public:
        GraphBuilder(const std::vector<size_t> &action_vec) : action_vec{action_vec} {}
        ~GraphBuilder() = default;

        /// @brief Creates a decision graph for pacing up and down in a MazeWorld.
        /// Assumes that the inputs are in the format: prev_action, current_state, above_state, below_state, left_state, right_state
        /// And assumes that the action outputs are in the format: up, down, left, right
        /// @param action_vec
        /// @return
        std::unique_ptr<Graph> VerticalPacer()
        {
            auto decision_graph = std::make_unique<Graph>(action_vec);

            std::vector<std::shared_ptr<GraphNode>> input_layer;
            std::shared_ptr<GraphNode> prev_action = std::make_shared<GraphNode>(0);
            std::shared_ptr<GraphNode> current_state = std::make_shared<GraphNode>(0);
            std::shared_ptr<GraphNode> above_state = std::make_shared<GraphNode>(0);
            std::shared_ptr<GraphNode> below_state = std::make_shared<GraphNode>(0);
            std::shared_ptr<GraphNode> left_state = std::make_shared<GraphNode>(0);
            std::shared_ptr<GraphNode> right_state = std::make_shared<GraphNode>(0);
            input_layer.insert(input_layer.end(), {prev_action, current_state, above_state, below_state, left_state, right_state});
            decision_graph->AddLayer(input_layer);

            // state == 1 => floor which is walkable
            std::vector<std::shared_ptr<GraphNode>> obstruction_layer;
            std::shared_ptr<GraphNode> up_not_blocked = std::make_shared<AnyEqNode>();
            up_not_blocked->AddInputs(std::vector<std::shared_ptr<GraphNode>>{above_state, std::make_shared<GraphNode>(1)});
            std::shared_ptr<GraphNode> down_not_blocked = std::make_shared<AnyEqNode>();
            down_not_blocked->AddInputs(std::vector<std::shared_ptr<GraphNode>>{below_state, std::make_shared<GraphNode>(1)});
            obstruction_layer.insert(obstruction_layer.end(), {up_not_blocked, down_not_blocked});
            decision_graph->AddLayer(obstruction_layer);

            // Separate previous action into up and down nodes
            std::vector<std::shared_ptr<GraphNode>> prev_action_layer;
            std::shared_ptr<GraphNode> up_prev_action = std::make_shared<AnyEqNode>();
            up_prev_action->AddInputs(std::vector<std::shared_ptr<GraphNode>>{prev_action, std::make_shared<GraphNode>(1)});
            std::shared_ptr<GraphNode> down_prev_action = std::make_shared<AnyEqNode>();
            down_prev_action->AddInputs(std::vector<std::shared_ptr<GraphNode>>{prev_action, std::make_shared<GraphNode>(2)});
            prev_action_layer.insert(prev_action_layer.end(), {up_prev_action, down_prev_action});
            decision_graph->AddLayer(prev_action_layer);

            std::vector<std::shared_ptr<GraphNode>> moving_layer;
            // If up_not_blocked and up_prev_action ? return 1 : return 0
            // If down_not_blocked and down_prev_action ? return 1 : return 0
            std::shared_ptr<GraphNode> keep_up = std::make_shared<AndNode>();
            keep_up->AddInputs(std::vector<std::shared_ptr<GraphNode>>{up_not_blocked, up_prev_action});
            std::shared_ptr<GraphNode> keep_down = std::make_shared<AndNode>();
            keep_down->AddInputs(std::vector<std::shared_ptr<GraphNode>>{down_not_blocked, down_prev_action});
            moving_layer.insert(moving_layer.end(), {keep_up, keep_down});
            decision_graph->AddLayer(moving_layer);

            // If down_blocked, turn_up
            // If up_blocked, turn_down
            std::vector<std::shared_ptr<GraphNode>> turn_layer;
            std::shared_ptr<GraphNode> turn_up = std::make_shared<NotNode>();
            turn_up->AddInputs(std::vector<std::shared_ptr<GraphNode>>{down_not_blocked});
            std::shared_ptr<GraphNode> turn_down = std::make_shared<NotNode>();
            turn_down->AddInputs(std::vector<std::shared_ptr<GraphNode>>{up_not_blocked});
            turn_layer.insert(turn_layer.end(), {turn_up, turn_down});
            decision_graph->AddLayer(turn_layer);

            // Output layer, up, down, left, right
            std::vector<std::shared_ptr<GraphNode>> action_layer;
            // move up = keep_up + turn_up,
            // move down = keep_down + turn_down,
            std::shared_ptr<GraphNode> up = std::make_shared<SumNode>();
            up->AddInputs(std::vector<std::shared_ptr<GraphNode>>{keep_up, turn_up});
            std::shared_ptr<GraphNode> down = std::make_shared<SumNode>();
            down->AddInputs(std::vector<std::shared_ptr<GraphNode>>{keep_down, turn_down});
            std::shared_ptr<GraphNode> left = std::make_shared<GraphNode>(0);
            std::shared_ptr<GraphNode> right = std::make_shared<GraphNode>(0);
            action_layer.insert(action_layer.end(), {up, down, left, right});
            decision_graph->AddLayer(action_layer);

            return decision_graph;
        }
    };
}
