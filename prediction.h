#ifndef PREDICTION_H
#define PREDICTION_H
#include<unordered_map>
#include <string>
#include <vector>
#include <memory>
#include <utility> // For std::pair
#include <iosfwd>  // For std::ostream and std::istream forward declarations

namespace FertilizerPrediction {

// Data structures
struct FertilizerDataPoint {
    double temperature;
    double humidity;
    double moisture;
    std::string soilType;
    std::string cropType;
    double nitrogen;
    double potassium;
    double phosphorous;
    std::string fertilizer;
};

struct PredictionResult {
    std::string fertilizer;
    double confidence;
};

// Classes
class FeatureEncoder;
class DecisionTreeClassifier;
class RandomForestClassifier;
class FertilizerPredictionSystem;

class FeatureEncoder {
public:
    FeatureEncoder();
    int encodeSoilType(const std::string& soilType);
    int encodeCropType(const std::string& cropType);
    int encodeFertilizer(const std::string& fertilizer);
private:
    friend class DecisionTreeClassifier; // Allow access for internal logic
    std::unordered_map<std::string, int> soilTypeMap;
    std::unordered_map<std::string, int> cropTypeMap;
    std::unordered_map<std::string, int> fertilizerMap;
    int nextSoilTypeCode;
    int nextCropTypeCode;
    int nextFertilizerCode;
};

class DecisionTreeClassifier {
public:
    explicit DecisionTreeClassifier(FeatureEncoder& encoder);
    void train(const std::vector<FertilizerDataPoint>& data);
    std::string predict(double t, double h, double m, const std::string& s, const std::string& c, double n, double k, double p);
    void saveModel(std::ostream& out) const;
    void loadModel(std::istream& in);
private:
    struct DecisionTreeNode;
    FeatureEncoder& encoder;
    std::shared_ptr<DecisionTreeNode> root;
};

class RandomForestClassifier {
public:
    explicit RandomForestClassifier(FeatureEncoder& encoder);
    void train(const std::vector<FertilizerDataPoint>& data);
    std::pair<std::vector<PredictionResult>, std::string> predictWithConfidence(const std::string& input);
    void saveModel(std::ostream& out) const;
    void loadModel(std::istream& in);
private:
    std::vector<DecisionTreeClassifier> trees;
    FeatureEncoder& encoder;
};

class FertilizerPredictionSystem {
public:
    FertilizerPredictionSystem();
    ~FertilizerPredictionSystem();
    bool loadDataFromCSV(const std::string& filename);
    bool trainModel();
    bool isModelReady() const;
    std::pair<std::vector<PredictionResult>, std::string> predict(const std::string& input);
    bool saveModel(const std::string& filename);
    bool loadModel(const std::string& filename);
private:
    FeatureEncoder encoder;
    std::unique_ptr<RandomForestClassifier> model;
    std::vector<FertilizerDataPoint> trainingData;
    bool isModelTrained;
};

} // namespace FertilizerPrediction

#endif // PREDICTION_H
