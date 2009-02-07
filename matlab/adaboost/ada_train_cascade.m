% load the parameters and path information
% ----------
ada_settings; 
ada_versioninfo;


%% preparation

% initialize the log file
logfile(FILES.log_filenm, 'erase');logfile(FILES.log_filenm, 'header', {INFO.appname, ['Version ' INFO.version], ['by ' INFO.author ', ' INFO.email], [num2str(TRAIN_POS) ' positive examples, ' num2str(TRAIN_NEG) ' negative examples.'], ['DATASETS from ' DATASETS.filelist], ['Started at ' datestr(now)], INFO.copyright, '-----------------------------------'});
logfile(FILES.log_filenm, 'column_labels', {'stage', 'step', 'Weak ID', 'Di', 'Fi', 'di', 'fi', 'di(train)', 'fi(train)'});

% define the weak learners
tic; disp('...defining the weak learners.');
WEAK = ada_define_weak_learners(LEARNERS); toc; 

% collect the training set and precompute feature responses
tic; disp('...collecting and processing the TRAIN data set.');
TRAIN = ada_collect_data(DATASETS, 'train'); toc;
TRAIN = ada_precompute(TRAIN, LEARNERS, WEAK, FILES, FILES.train_filenm);

% collect the validation set and precompute feature responses
tic; disp('...collecting and processing the VALIDATION data set.');
VALIDATION = ada_collect_data(DATASETS, 'validation'); toc;
VALIDATION = ada_precompute(VALIDATION, LEARNERS, WEAK, FILES, FILES.valid_filenm);


%% train the cascade

CASCADE = ada_cascade_init();   % initialize the CASCADE struct
i = 0;                          % cascade stage index
Fi = 1;                         % cascade's current false positive rate      
Di = 1;                         % cascade's current detection rate

% loop until we meet the target false positive rate
while (Fi > Ftarget)
    i = i + 1;
    disp(['============== NOW TRAINING CASCADE STAGE i = ' num2str(i) ' ==============']);
    ti = 0;                     % weak learner index (within current stage)

    if i == 1; Flast = 1; else Flast = prod([CASCADE(1:i-1).fi]); end
    if i == 1; Dlast = 1; else Dlast = prod([CASCADE(1:i-1).di]); end
     
    % cascade must meet detection and FP rate goals before passing to the next stage
    while (Fi > fmax * Flast) || (Di < dmin * Dlast)
        ti = ti + 1;

        
        %% train the next weak classifier (for the strong classifier of stage i)
        disp('   ----------------------------------------------------------------------');
        disp(['...CASCADE stage ' num2str(i) ' training classifier hypothesis ti=' num2str(ti) '.']);
        if ti == 1
            CASCADE(i).CLASSIFIER = ada_adaboost(TRAIN, WEAK, ti, LEARNERS);
        else
            CASCADE(i).CLASSIFIER = ada_adaboost(TRAIN, WEAK, ti, LEARNERS, CASCADE(i).CLASSIFIER);
        end
        
        %% select the cascade threshold for stage i
        %  adjust the threshold for the current stage until we find one
        %  which gives a satifactory detection rate (this changes the false alarm rate)
        [CASCADE, Fi, Di]  = ada_cascade_select_threshold(CASCADE, i, VALIDATION, dmin);

        % ====================  TEMPORARY  ==============================
        % to make sure we're actually improving on the training data
        gt = [TRAIN(:).class]'; 
        C = ada_classify_set(CASCADE, TRAIN);
        [tpr fpr FPs] = rocstats(C, gt, 'TPR', 'FPR', 'FPlist');
        disp(['results on TRAIN data for CASCADE: Di=' num2str(tpr) ', (f)i=' num2str(fpr) ', #FP = ' num2str(length(FPs)) ' (remember class 0 = FP)' ]);               
        % ===============================================================
        
        % write training results to the log file
        logfile(FILES.log_filenm, 'write', [i ti CASCADE(i).CLASSIFIER.feature_index(ti) Di Fi CASCADE(i).di CASCADE(i).fi tpr fpr]);
        
        % save the cascade to a file in case something bad happens and we need to restart
        save(FILES.cascade_filenm, 'CASCADE');
        disp(['...saved a temporary copy of CASCADE to ' FILES.cascade_filenm]);
        
        % hand-tune some stage goals (for first few stages) loosly following Viola-Jones IJCV '04
        if i == 1; if (CASCADE(i).di >= .99) && (CASCADE(i).fi < .50);   break; end; end;
        if i == 2; if (CASCADE(i).di >= .99) && (CASCADE(i).fi < .40);   break; end; end;
        if i == 3; if (Di > dmin * Dlast) && (Fi < Flast*.35);  break; end; end;
        if i == 4; if (Di > dmin * Dlast) && (Fi < Flast*.35);  break; end; end;
    end
    
    %% prepare training & validation data for the next stage of the cascade  
    %  recollect negative examples for the training and validation set which 
    %  include only FPs generated by the current cascade

    disp('...updating the TRAIN set with negative examples which cause false positives');
    TRAIN = ada_collect_data(DATASETS, 'update', TRAIN, CASCADE, LEARNERS);
    TRAIN = ada_recompute(TRAIN, LEARNERS, WEAK, FILES);
    
    disp('...updating the VALIDATION set with negative examples which cause false positives');
    VALIDATION = ada_collect_data(DATASETS, 'update', VALIDATION, CASCADE, LEARNERS);
    VALIDATION = ada_recompute(VALIDATION, LEARNERS, WEAK, FILES);
    
%     TRAIN = ada_collect_data(DATASETS, LEARNERS, 'train', 'update', TRAIN, CASCADE);
%     TRAIN = ada_precompute(TRAIN, LEARNERS, WEAK, FILES, FILES.train_filenm);
%                 
%     disp('...updating the VALIDATION set with negative examples which cause false positives');
%     VALIDATION = ada_collect_data(DATASETS, LEARNERS, 'validation', 'update', VALIDATION, CASCADE);
%     VALIDATION = ada_precompute(VALIDATION, LEARNERS, WEAK, FILES, FILES.valid_filenm);
     
end


%% training goals achieved, clean up and quit!
disp('');
disp('==============================================================================');
disp(['Training complete.  CASCADE is stored in ' cascade_filenm '.']);
disp('==============================================================================');
clear TRAIN VALIDATION C gt NORM WEAK Di dmin Dlast Fi fmax Flast tpr fpr Ftarget FPs i j ti log_filenm appname version author email IMSIZE TRAIN_POS TRAIN_NEG TEST_POS TEST_NEG cascade_filenm temppath temp_filenm datapath train1 train0 validation1 validation0 update0


