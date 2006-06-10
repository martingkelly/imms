#include <torch/MatDataSet.h>
#include <torch/ClassFormatDataSet.h>
#include <torch/ClassNLLCriterion.h>
#include <torch/MSECriterion.h>
#include <torch/OneHotClassFormat.h>
#include <torch/ClassMeasurer.h>
#include <torch/MSEMeasurer.h>

#include <torch/StochasticGradient.h>
#include <torch/KFold.h>

#include <torch/ConnectedMachine.h>
#include <torch/Linear.h>
#include <torch/Tanh.h>
#include <torch/LogSoftMax.h>

#include <torch/MeanVarNorm.h>
#include <torch/NullXFile.h>
#include <torch/CmdLine.h>
#include <torch/Random.h>

#include "model/model.h"
#include "immscore/immsutil.h"

#include <string>
#include <iostream>
#include <assert.h>

using namespace Torch;
using namespace std;

const string AppName = "train_model";

int main(int argc, char **argv)
{
  char *file;

  int n_inputs = NUM_FEATURES;
  int n_targets = 2;
  int k_fold;

  real accuracy;
  real learning_rate;
  real decay = 0;
  int max_iter = 100;

  char *model_file;
  real weight_decay;
  int n_outputs;

  Allocator *allocator = new Allocator;
  DiskXFile::setLittleEndianMode();

  //=================== The command-line ==========================

  // Construct the command line
  CmdLine cmd;

  // Train mode
  cmd.addText("\nArguments:");
  cmd.addSCmdArg("file", &file, "the train file");

  cmd.addText("\nLearning Options:");
  cmd.addRCmdOption("-lr", &learning_rate, 0.01, "learning rate");
  cmd.addRCmdOption("-e", &accuracy, 0.0001, "end accuracy");
  cmd.addICmdOption("-kfold", &k_fold, -1, "number of folds, if you want to do cross-validation");
  cmd.addRCmdOption("-wd", &weight_decay, 0, "weight decay", true);

  cmd.addText("\nMisc Options:");
  cmd.addSCmdOption("-save", &model_file, "", "the model file");

  // Test mode
  cmd.addMasterSwitch("--test");
  cmd.addText("\nArguments:");
  cmd.addSCmdArg("model", &model_file, "the model file");
  cmd.addSCmdArg("file", &file, "the test file");

  // Read the command line
  int mode = cmd.read(argc, argv);

  DiskXFile *model = NULL;
  if(mode == 1)
  {
    model = new(allocator) DiskXFile(model_file, "r");
    cmd.loadXFile(model);
  }

  // If the user didn't give any random seed,
  // generate a random random seed...
  if(mode == 0)
      Random::seed();
  cmd.setWorkingDirectory(".");

  n_outputs = n_targets;

  //=================== Create the MLP... =========================
  ConnectedMachine mlp;

  int n_hu = 2 * n_inputs;
  Linear *c1 = new(allocator) Linear(n_inputs, n_hu);
  c1->setROption("weight decay", weight_decay);
  Tanh *c2 = new(allocator) Tanh(n_hu);
  Linear *c3 = new(allocator) Linear(n_hu, n_outputs);
  c3->setROption("weight decay", weight_decay);

  mlp.addFCL(c1);
  mlp.addFCL(c2);
  mlp.addFCL(c3);

  LogSoftMax *c4 = new(allocator) LogSoftMax(n_outputs);
  mlp.addFCL(c4);

  // Initialize the MLP
  mlp.build();
  mlp.setPartialBackprop();

  //=================== DataSets & Measurers... ===================

  // Create the training dataset (normalize inputs)
  MatDataSet *mat_data = new(allocator) MatDataSet(
          file, n_inputs, 1, false, -1, false);
  MeanVarNorm *mv_norm = new(allocator) MeanVarNorm(mat_data);
  if(mode == 1)
      mv_norm->loadXFile(model);
  mat_data->preProcess(mv_norm);

  DataSet *data = new(allocator) ClassFormatDataSet(mat_data, n_targets);

  if(mode == 1)
    mlp.loadXFile(model);

  // The list of measurers...
  MeasurerList measurers;

  // The class format
  OneHotClassFormat *class_format = new(allocator) OneHotClassFormat(n_outputs);

  // Measurers on the training dataset
  ClassMeasurer *class_meas = new(allocator)
      ClassMeasurer(mlp.outputs, data, class_format,
              cmd.getXFile("class_error"), false);
  measurers.addNode(class_meas);
  
  //=================== The Trainer ===============================
  
  // The criterion for the StochasticGradient (MSE criterion or NLL criterion)
  Criterion *criterion = new(allocator) ClassNLLCriterion(class_format);

  // The Gradient Machine Trainer
  StochasticGradient trainer(&mlp, criterion);
  if(mode == 0)
  {
    trainer.setIOption("max iter", max_iter);
    trainer.setROption("end accuracy", accuracy);
    trainer.setROption("learning rate", learning_rate);
    trainer.setROption("learning rate decay", decay);
  }

  //=================== Let's go... ===============================

  // Print the number of parameter of the MLP (just for fun)
  message("Number of parameters: %d", mlp.params->n_params);

  if(mode == 0)
  {
    if(k_fold <= 0)
    {
      trainer.train(data, &measurers);
      
      if(strcmp(model_file, ""))
      {
        DiskXFile all_(model_file, "w");
        DiskXFile model_("file.model", "w");
        DiskXFile norm_("file.norm", "w");
        DiskXFile cmd_("file.cmd", "w");
        cmd.saveXFile(&cmd_);
        mv_norm->saveXFile(&norm_);
        mlp.saveXFile(&model_);
        cmd.saveXFile(&all_);
        mv_norm->saveXFile(&all_);
        mlp.saveXFile(&all_);
      }
    }
    else
    {
      KFold k(&trainer, k_fold);
      k.crossValidate(data, NULL, &measurers);
    }
  }
  else {
    SimilarityModel m;

    float correct = 0, wrong = 0;
    for (int t = 0; t < data->n_examples; t++) {
        data->setExample(t);

        mlp.forward(data->inputs);
        int true_class = (int)data->targets->frames[0][1];
        int obs_class = class_format->getClass(mlp.outputs->frames[0]);
        float score = mlp.outputs->frames[0][1] - mlp.outputs->frames[0][0];
        score /= 5.;
        (true_class == obs_class ? correct : wrong) += 1;

        float model_score = m.evaluate(data->inputs->frames[0], false);
        assert(fabs(score - model_score) < 0.01);
        assert(obs_class == 0 || score >= 0);
        assert(obs_class == 1 || score <= 0);
    }
    cout << "TOTAL      : " << correct + wrong << endl;
    cout << "CORRECT    : " << correct << endl;
    cout << "WRONG      : " << wrong << endl;
    cout << "ERROR      : " << wrong / (correct + wrong) << endl;
  }

  delete allocator;

  return(0);
}
