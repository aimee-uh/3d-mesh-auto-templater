#include "solver.h"


//==========[ Function Prototypes ]============================================

	// Fortran method for LBFGS solver
	// changed to C solver
#ifndef WIN32
#define SETULB setulb_
#endif
/*extern "C" { extern void setulb_(int*,int*,double*,double*,double*,int*,
								double*,double*,double*,double*,double*,
								int*,char*,int*,char*,int*,int*,double* ); }
*/

extern "C" {
	extern void lbfgsb(int *n, int *m, double *x, double *l, double *u, int *nbd, 
					   double *f, double *g, double *factr, double *pgtol, double *wa, int *iwa, 
					   char *task, int *iprint, char *csave, int *lsave, int *isave, double *dsave, short task_len, short csave_len); 
}

//==========[ LBFGSSolver Methods ]============================================

LBFGSSolver::LBFGSSolver( IDifferentiableFunction* function ) {
	mFunction = function;

	mWorkspace = NULL;
	mIntWorkspace = NULL;
	mActiveBounds = NULL;

	mTask = new char[60];
	mCharSave = new char[60];
	mBoolSave = new int[4];
	mIntSave = new int[44];
	mDoubleSave = new double[29];

	mNumVars = 0;

	stopNow = false;
}

LBFGSSolver::~LBFGSSolver() {
	delete [] mTask;
	delete [] mCharSave;
	delete [] mBoolSave;
	delete [] mIntSave;
	delete [] mDoubleSave;

	if( mWorkspace != NULL )
		delete [] mWorkspace;

	if( mIntWorkspace != NULL )
		delete [] mIntWorkspace;

	if( mActiveBounds != NULL )
		delete [] mActiveBounds;
}

void LBFGSSolver::resize( int size ) {
	if( mNumVars < size ) {
		mGradient.resize( size );
		mLowerBounds.resize( size );
		mUpperBounds.resize( size );

		if( mWorkspace != NULL ) delete [] mWorkspace;
		if( mIntWorkspace != NULL ) delete [] mIntWorkspace;
		if( mActiveBounds != NULL ) delete [] mActiveBounds;

		mWorkspace = new double[14*size+315];
		mIntWorkspace = new int[3*size];
		mActiveBounds = new int[size];

		int i;
		for( i=0;i<size;i++ )
			mActiveBounds[i] = 0;

		mNumVars = size;
	}
}

double LBFGSSolver::solve( double factr, double pgtol, Vecd& x, int maxIterations ) {
	int			i;
	int			n = x.size();
	int			m = 5;
	double		f = 0;
	int			iprint = -1;
	bool		converged = false;
	int         numIterations = 0;

	resize( n );

	strcpy( mTask, "START" );
	for( i=5;i<60;i++ )
		mTask[i] = ' ';

//	for( i=0;i<n;i++ )
//		mActiveBounds[i] = 0;

	while( !converged ) {
//		cout << ".";
/*		setulb_( &n, &m, x.getPointer(), mLowerBounds.getPointer(), mUpperBounds.getPointer(),
				mActiveBounds, &f, mGradient.getPointer(), &factr, &pgtol,
				mWorkspace, mIntWorkspace, mTask, &iprint, mCharSave,
				mBoolSave, mIntSave, mDoubleSave );*/
//		cout << "start lbfgsb here" << n <<endl;
		lbfgsb( &n, &m, x.getPointer(), mLowerBounds.getPointer(), mUpperBounds.getPointer(),
				mActiveBounds, &f, mGradient.getPointer(), &factr, &pgtol,
				mWorkspace, mIntWorkspace, mTask, &iprint, mCharSave,
				mBoolSave, mIntSave, mDoubleSave,60,60);
//		cout << "finish lbfgsb here" <<endl;

		if( mTask[0] == 'F' && mTask[1] == 'G' ) {
			f = mFunction->evaluateFunction( x );
			mFunction->evaluateGradient( x, mGradient );
		} else if( mTask[0] == 'N' && mTask[1] == 'E' &&
					mTask[2] == 'W' && mTask[3] == '_' &&
					mTask[4] == 'X' ) {
			converged = false;
			mFunction->solverStep();
			if (stopNow)
				break;
			numIterations++;
			if (numIterations >= maxIterations) {
				break;
			}
		} else {
			converged = true;
		}
	}
/*
	if (converged)
		cout << "solver converged." << endl;
	else
		cout << "solve stopped." << endl;
*/
	return f;
}

void LBFGSSolver::setBound(int index, double lower, double upper) {
	mActiveBounds[index] = 2;
	mLowerBounds[index] = lower;
	mUpperBounds[index] = upper;
}