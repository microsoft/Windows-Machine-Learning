winrt::build!(
    dependencies
        nuget: Microsoft.Windows.SDK.Contracts
        nuget: Microsoft.AI.MachineLearning
    types
        microsoft::ai::machine_learning::*
        windows::graphics::imaging::*
);

fn main() {
    build();
}
