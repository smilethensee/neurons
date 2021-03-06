%%load the parameters and path information
% ----------
ada_settings;
%insunrays3b_settings;
ada_versioninfo;
ada_stage_goals;

%% preparation

% initialize the log file
logfile(FILES.log_filenm, 'erase');logfile(FILES.log_filenm, 'header', {INFO.appname, ['Version ' INFO.version], ['by ' INFO.author ', ' INFO.email], [num2str(TRAIN_POS) ' positive examples, ' num2str(TRAIN_NEG) ' negative examples.'], ['DATASETS from ' DATASETS.filelist], ['LEARNERS ' LEARNERS(:).feature_type],['Started at ' datestr(now)], INFO.copyright, '-----------------------------------'});
logfile(FILES.log_filenm, 'column_labels', {'stage', 'step', 'Weak ID', 'Di', 'Fi', 'di', 'fi', 'di(train)', 'fi(train)', 'FPs', 'LEARNER'});

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

CASCADE = ada_cascade_init(DATASETS);       % initialize the CASCADE struct
i = 0;                                      % cascade stage index
Fi = 1;                                     % cascade's current false positive rate      
Di = 1;                                     % cascade's current detection rate


while (Fi > Ftarget)            % loop until we meet the target false positive rate
    i = i + 1;
    disp(['============== NOW TRAINING CASCADE STAGE i = ' num2str(i) ' ==============']);
    ti = 0;                     % weak learner index (within current stage)

    if i == 1; Flast = 1; else Flast = prod([CASCADE(1:i-1).fi]); end
    if i == 1; Dlast = 1; else Dlast = prod([CASCADE(1:i-1).di]); end
    CASCADE(i).di = 0;  CASCADE(i).fi = 0;  restart_flag = 0;
    
    % cascade must meet detection and FP rate goals before passing to the next stage
    %while (Fi > GOALS(i).fmax * Flast) || (Di < dmin * Dlast)
    while (CASCADE(i).fi > GOALS(i).fmax) || (CASCADE(i).di < GOALS(i).dmin)
        ti = ti + 1;

        %% train the next weak classifier (for the strong classifier of stage i)
        disp('   ----------------------------------------------------------------------');
        disp(['...CASCADE stage ' num2str(i) ' selecting weak learner ti=' num2str(ti) '.']);
        if ti == 1
            [CASCADE(i).CLASSIFIER, restart_flag] = ada_adaboost(TRAIN, WEAK, ti, LEARNERS);
        else
            [CASCADE(i).CLASSIFIER, restart_flag] = ada_adaboost(TRAIN, WEAK, ti, LEARNERS, CASCADE(i).CLASSIFIER);
        end
        
        
        disp(['Stage ' num2str(i) ' goals: min detection rate (di) > ' num2str(GOALS(i).dmin) ', max false positive rate (fi) < ' num2str(GOALS(i).fmax) '.']);
        
        %% select the cascade threshold for stage i
        %  adjust the threshold for the current stage until we find one
        %  which gives a satifactory detection rate (this changes the false alarm rate)
        [CASCADE, Fi, Di]  = ada_cascade_select_threshold(CASCADE, i, VALIDATION, GOALS(i).dmin);

        % ====================  TEMPORARY  ==============================
        % to make sure we're actually improving on the training data
        gt = [TRAIN(:).class]';  C = ada_classify_set(CASCADE, TRAIN);
        [tpr fpr FPs TNs] = rocstats(C, gt, 'TPR', 'FPR', 'FPlist', 'TNlist');
        disp(['Di=' num2str(tpr) ', Fi=' num2str(fpr) ', #FP = ' num2str(length(FPs)) '.  CASCADE applied to TRAIN set.'  ]);               
        % ===============================================================
        
        % ============== HACK TO RESTART THE STAGE IF REPEATING CLASSIFIER ==========
        if restart_flag
            i = i - 1;   if i > 0; CASCADE = CASCADE(1:i); end; break;
        end
        % ===========================================================================

        % write training results to the log file
        for l = 1:length(LEARNERS); if strcmp(CASCADE(i).CLASSIFIER.weak_learners{ti}.type, LEARNERS(l).feature_type); L_ind = l; end; end;
        logfile(FILES.log_filenm, 'write', [i ti CASCADE(i).CLASSIFIER.feature_index(ti) Di Fi CASCADE(i).di CASCADE(i).fi tpr fpr length(FPs) L_ind]);
        
        % save the cascade to a file in case something bad happens and we need to restart
        save(FILES.cascade_filenm, 'CASCADE', 'FILES', 'DATASETS', 'LEARNERS'); disp(['       ...saved a temporary copy of CASCADE to ' FILES.cascade_filenm]);
    end
    
    %% check to see if we have completed training
    if (Fi <= prod([GOALS(:).fmax])) && (Di >= prod([GOALS(:).dmin]))
        break;
    end

    if ~restart_flag
        %% prepare training & validation data for the next stage of the cascade  
        %  recollect negative examples for the training and validation set which 
        %  include only FPs generated by the current cascade
        disp('       ...updating the TRAIN set with negative examples which cause false positives');
        TRAIN = ada_collect_data(DATASETS, 'update', TRAIN, CASCADE, LEARNERS);
        TRAIN = ada_precompute(TRAIN, LEARNERS, WEAK, FILES, 're');

        disp('       ...updating the VALIDATION set with negative examples which cause false positives');
        VALIDATION = ada_collect_data(DATASETS, 'update', VALIDATION, CASCADE, LEARNERS);
        VALIDATION = ada_precompute(VALIDATION, LEARNERS, WEAK, FILES, 're');
    else
        % if we need to restart the stage we were just on
        disp('       ...updating the TRAIN NEG set with a new set of false positives');
        TRAIN = ada_collect_data(DATASETS, 'recollectFPs', TRAIN, CASCADE, LEARNERS);
        TRAIN = ada_precompute(TRAIN, LEARNERS, WEAK, FILES, 're');
        disp('       ...updating the VALIDATION NEG set with a new set of false positives');
        VALIDATION = ada_collect_data(DATASETS, 'recollectFPs', VALIDATION, CASCADE, LEARNERS);
        VALIDATION = ada_precompute(VALIDATION, LEARNERS, WEAK, FILES ,'re');
    end
    
end


%% training goals achieved, clean up and quit!
disp('');
disp('==============================================================================');
disp(['Training complete.  CASCADE is stored in ' FILES.cascade_filenm '.']);
disp('==============================================================================');
%clear TRAIN VALIDATION C gt NORM WEAK Di dmin Dlast Fi fmax Flast tpr fpr Ftarget FPs i j ti log_filenm appname version author email IMSIZE TRAIN_POS TRAIN_NEG TEST_POS TEST_NEG cascade_filenm temppath temp_filenm datapath train1 train0 validation1 validation0 update0


