Eigen::VectorXd PCDA_predictions(Eigen::VectorXd w, double HEIGHT, int mf) {
    
    //MALE
    
    if (mf == 0) {
    double WBTOT_FAT = 19.71 + (-0.21) * w[0]  + 0.39 * w[1] + (-2.68 * w[2]) + 0.92 * w[3] + (-0.68 * w[4]) + 1.52 * w[7] + 1.42 * w[8] + 1.03 * w[9] + 1.81 * w[12] + 1.06 * w[14];
    
    double WBTOT_MASS = 85.83
                        -0.88 * w[0]
                        +0.92 * w[1]
                        -5.53 * w[2]
                        +2.15 * w[3]
                        -1.17 * w[4]
                        +0.93 * w[5]
                        +0.40 * w[6]
                        +1.40 * w[7]
                        +1.14 * w[8]
                        +0.80 * w[9]
                        -0.77 * w[10]
                        +0.83 * w[12]
                        -2.06 * w[14];

    double WBTOT_LEAN = WBTOT_MASS - WBTOT_FAT;
    double WBTOT_PFAT = WBTOT_FAT / WBTOT_MASS;
    double FMI = WBTOT_FAT / (HEIGHT * HEIGHT);
    double LMI = WBTOT_LEAN / (HEIGHT * HEIGHT);
    double VFAT_MASS = 0.46
                       +0.01 * w[1]
                       -0.05 * w[2]
                       -0.02 * w[5]
                       +0.05 * w[7]
                       +0.03 * w[8]
                       -0.06 * w[11]
                       +0.06 * w[12]
                       +0.14 * w[14];

    double TRUNK_FAT = 9.66
                       -0.10 * w[0]
                       +0.27 * w[1]
                       -1.57 * w[2]
                       +0.43 * w[3]
                       -0.32 * w[4]
                       -0.19 * w[5]
                       +0.96 * w[7]
                       +0.75 * w[8]
                       +0.42 * w[9]
                       +0.40 * w[10]
                       +0.97 * w[12]
                       +1.37 * w[14];
   
    double TRUNK_LEAN = 31.49
                        -0.31 * w[0]
                        +0.27 * w[1]
                        -1.49 * w[2]
                        +0.60 * w[3]
                        -0.19 * w[4]
                        +0.39 * w[5]
                        -0.54 * w[10]
                        -0.74 * w[14];
                 
    double ARM_FAT = 1.19
                     -0.01 * w[0]
                     +0.02 * w[1]
                     -0.17 * w[2]
                     +0.07 * w[3]
                     -0.04 * w[4]
                     +0.09 * w[7]
                     +0.09 * w[8]
                     +0.08 * w[9]
                     +0.12 * w[12];

    double ARM_LEAN = 4.39 - 0.05 * w[0] + 0.05 * w[1] - 0.19 * w[2] + 0.09 * w[3] + 0.11 * w[5] + 0.07 * w[6] - 0.08 * w[8] - 0.12 * w[10] - 0.15 * w[12] - 0.35 * w[14];

    double LEG_FAT = 3.27
                     -0.04 * w[0]
                     -0.38 * w[2]
                     +0.17 * w[3]
                     -0.13 * w[4]
                     +0.19 * w[7]
                     +0.23 * w[8]
                     +0.20 * w[9]
                     +0.20 * w[11]
                     +0.30 * w[12];
                     
    double LEG_LEAN = 10.81
                      -0.12 * w[0]
                      +0.07 * w[1]
                      -0.45 * w[2]
                      +0.23 * w[3]
                      -0.12 * w[4]
                      +0.21 * w[5]
                      +0.15 * w[6]
                      -0.28 * w[10]
                      -0.23 * w[12]
                      -0.78 * w[14];
                      
    double APP_LMI = (ARM_LEAN + LEG_LEAN) / (HEIGHT * HEIGHT);  
    Eigen::VectorXd predictions(13);
    predictions << WBTOT_PFAT, WBTOT_LEAN, WBTOT_FAT, VFAT_MASS, LEG_LEAN, ARM_LEAN, APP_LMI, FMI, LMI, TRUNK_FAT, TRUNK_LEAN, ARM_FAT, LEG_FAT;
    return predictions;
    }
    else {
    double WBTOT_FAT = 24.53
                       -0.06 * w[0]
                       +0.33 * w[1]
                       -3.27 * w[2]
                       +0.43 * w[3]
                       -0.45 * w[7]
                       +0.89 * w[8]
                       -1.04 * w[10];

    double WBTOT_MASS = 69.86
                        -0.35 * w[0]
                        +0.56 * w[1]
                        -5.57 * w[2]
                        +0.88 * w[3]
                        -0.20 * w[5]
                        -0.25 * w[6]
                        -1.09 * w[7]
                        +0.71 * w[8]
                        +1.75 * w[9]
                        -0.73 * w[10]
                        +0.87 * w[13]
                        -1.67 * w[14];
    
    double WBTOT_LEAN = WBTOT_MASS - WBTOT_FAT;
    double WBTOT_PFAT = WBTOT_FAT / WBTOT_MASS;
    double FMI = WBTOT_FAT / (HEIGHT * HEIGHT);
    double LMI = WBTOT_LEAN / (HEIGHT * HEIGHT);
    
    double VFAT_MASS = 0.42
                       +0.00
                       +0.01 * w[1]
                       -0.07 * w[2]
                       -0.02 * w[6]
                       -0.03 * w[9]
                       +0.05 * w[12]
                       -0.04 * w[13]
                       +0.11 * w[14];
    
    double TRUNK_FAT = 11.15
                       +0.25 * w[1]
                       -1.71 * w[2]
                       +0.30 * w[3]
                       -0.28 * w[7]
                       +0.26 * w[8]
                       -0.45 * w[10]
                       +0.32 * w[12]
                       -0.30 * w[13]
                       +0.72 * w[14];
                       
    double TRUNK_LEAN = 22.45
                        -0.14 * w[0]
                        +0.13 * w[1]
                        -1.19 * w[2]
                        +0.20 * w[3]
                        +0.14 * w[4]
                        -0.35 * w[6]
                        -0.45 * w[7]
                        +0.57 * w[9];
    
    double ARM_FAT = 1.59
                     +0.03 * w[1]
                     -0.27 * w[2]
                     +0.05 * w[3];
                     
    double ARM_LEAN = 2.38
                      -0.02 * w[0]
                      +0.02 * w[1]
                      -0.13 * w[2]
                      +0.04 * w[3]
                      +0.06 * w[10]
                      -0.10 * w[14];
                      
    double LEG_FAT = 4.64
                     -0.03 * w[0]
                     -0.53 * w[2]
                     +0.24 * w[8]
                     -0.24 * w[10]
                     +0.40 * w[13]
                     -0.65 * w[14];
    
    double LEG_LEAN = 7.30
                      -0.06 * w[0]
                      +0.03 * w[1]
                      -0.39 * w[2]
                      +0.08 * w[3]
                      -0.09 * w[7]
                      +0.32 * w[9]
                      +0.17 * w[13]
                      -0.49 * w[14];
    
    double APP_LMI = (ARM_LEAN + LEG_LEAN) / (HEIGHT * HEIGHT);  
    Eigen::VectorXd predictions(13);
    predictions << WBTOT_PFAT, WBTOT_LEAN, WBTOT_FAT, VFAT_MASS, LEG_LEAN, ARM_LEAN, APP_LMI, FMI, LMI, TRUNK_FAT, TRUNK_LEAN, ARM_FAT, LEG_FAT;
    return predictions;

    }
}
