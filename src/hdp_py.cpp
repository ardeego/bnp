/* Copyright (c) 2012, Julian Straub <jstraub@csail.mit.edu>
 * Licensed under the MIT license. See LICENSE.txt or 
 * http://www.opensource.org/licenses/mit-license.php */

#include <baseMeasure.hpp>
#include <hdp.hpp>

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <typeinfo>

#include <armadillo>

#include <boost/python.hpp>
#include <boost/python/wrapper.hpp>
#include <python2.7/object.h> // for PyArray_FROM_O
#include <numpy/ndarrayobject.h> // for PyArrayObject

using namespace boost::python;

enum NpTypes{
  INT32 = 5,
  UINT32 = 6,
  INT64 = 7,
  UINT64 = 8,
  FLT32 = 11,
  FLT64 = 12, // default of python on my 64bit
}; // for value in PyArrayObject->descr->type_num

template<class U>
struct NpTyp
{
  const static NpTypes Num;
};
template<>
struct NpTyp<float>
{
  const static NpTypes Num=FLT32;
};
template<>
struct NpTyp<double>
{
  const static NpTypes Num=FLT64;
};
template<>
struct NpTyp<uint32_t>
{
  const static NpTypes Num=UINT32;
};
template<>
struct NpTyp<uint64_t>
{
  const static NpTypes Num=UINT64;
};
template<>
struct NpTyp<int32_t>
{
  const static NpTypes Num=INT32;
};
template<>
struct NpTyp<int64_t>
{
  const static NpTypes Num=INT64;
};

bool checkPyArr(PyArrayObject* a, const int ndims, const NpTypes npType)
{
	if (a == NULL) {
		//throw std::exception("Could not get NP array.");
		cerr<<"Could not get NP array."<<endl;
		return false;
//	}else if (a->descr->elsize != sizeof(double))
//	{
//		//throw std::exception("Must be double ndarray");
//		cerr<<"Must be double ndarray"<<endl;
//		return false;
	}else if(a->descr->type_num != npType)
	{
	  cerr<<"Wrong datatype on array ("<<a->descr->type_num<<" != "<<npType<<")"<<endl;
	  return false;
	}else if(a->descr->type_num == npType)
	{
	  if ((npType == FLT64 && a->descr->elsize != sizeof(double)) ||
	      (npType == FLT32 && a->descr->elsize != sizeof(float)) ||
	      (npType == UINT32 && a->descr->elsize != sizeof(uint32_t)) ||
	      (npType == UINT64 && a->descr->elsize != sizeof(uint64_t)) ||
	      (npType == INT32 && a->descr->elsize != sizeof(int32_t)) ||
	      (npType == INT64 && a->descr->elsize != sizeof(int64_t)))
	  {
	    cerr<<"Ensure that numpy datatype definitions are matching your architectures"<<endl;
	    return false;
	  }
	}else if (a->nd != ndims)
	{
		//throw std::exception("Wrong dimension on array.");
		cerr<<"Wrong dimension on array ("<<a->nd<<" != "<<ndims<<")"<<endl;
		return false;
	}else if (ndims == 2 && (a->strides[0] != a->dimensions[0] || a->strides[1] != 1))
		//((ndims == 1 && (a->strides[0] != 1))
	{
		cerr<<"Strides are not right (ndims="<< ndims <<" strides: "
				<< a->strides[0] <<"; "<< a->strides[1] <<")";
		return false;
	}

	//cout<<"Type="<<a->descr->type<< " Type_num=" <<a->descr->type_num<<endl;
	return true;
}

template<class U>
Mat<U> np2mat(const numeric::array& np)
{
  // Get pointer to np array
  PyArrayObject* a = (PyArrayObject*)PyArray_FROM_O(np.ptr());
  if(!checkPyArr(a,2,NpTyp<U>::Num)) exit(0);
  // do not copy the data!
  return arma::Mat<U>((U*)a->data,a->dimensions[0],a->dimensions[1],false,true);
}

template<class U>
Row<U> np2row(const numeric::array& np)
{
	// Get pointer to np array
	PyArrayObject* a = (PyArrayObject*)PyArray_FROM_O(np.ptr());
	if(!checkPyArr(a,1,NpTyp<U>::Num)) exit(0);
	// do not copy the data!
	return Row<U>((U*)a->data,a->dimensions[0],false,true);
}

template<class U>
Col<U> np2col(const numeric::array& np)
{
	// Get pointer to np array
	PyArrayObject* a = (PyArrayObject*)PyArray_FROM_O(np.ptr());
	if(!checkPyArr(a,1,NpTyp<U>::Num)) exit(0);
	// do not copy the data!
	return Col<U>((U*)a->data,a->dimensions[0],false,true);
}

class Dir_py : public Dir
{
public:
  Dir_py(const numeric::array& alphas) :
	  Dir(np2row<double>(alphas))
  {
	  cout<<"alphas: "<<mAlphas<<endl;
  };
  Dir_py(const Dir_py& dir) :
	  Dir(dir)
  {
	  cout<<"alphas: "<<mAlphas<<endl;
  };
};

class InvNormWishart_py : public InvNormWishart
{
public:
	InvNormWishart_py(const numeric::array& vtheta, double kappa,
			const numeric::array& Delta, double nu) :
        InvNormWishart(np2col<double>(vtheta),kappa,np2mat<double>(Delta),nu)
    //        cpVtheta(np2col<double>(vtheta)), cpDelta(np2mat<double>(Delta)),
//  InvNormWishart(cpVtheta,kappa,cpDelta,nu)
	{
	  cout<<"Creating "<<typeid(this).name()<<endl;
	};
	InvNormWishart_py(const InvNormWishart_py& inw) :
		InvNormWishart(inw)
	{};
private:
	colvec cpVtheta;
	mat cpDelta;
};

template <class U>
class HDP_py : public HDP<U>
{
public:
  HDP_py(const BaseMeasure<U>& base, double alpha, double gamma)
  : HDP<U>(base,alpha,gamma)
  {
    cout<<"Creating "<<typeid(this).name()<<endl;
  };

  bool densityEst(uint32_t K0=10, uint32_t T0=10, uint32_t It=10)
  {
    cout<<"mX.size()="<<HDP<U>::mX.size()<<endl;
    for (uint32_t i=0; i<HDP<U>::mX.size(); ++i)
      cout<<"  x_"<<i<<": "<<HDP<U>::mX[i].n_rows<<"x"<<HDP<U>::mX[i].n_cols<<": "<<HDP<U>::mX[i]<<endl;

    return HDP<U>::densityEst(K0, T0, It);
  }

  // makes no copy of the external data x_i
  uint32_t addDoc(const numeric::array& x_i)
  {
    Mat<U> x_i_mat=np2mat<U>(x_i); // can do this since x_i_mat gets copied inside

    cout<<"adding  x: "<<x_i_mat.n_rows<<"x"<<x_i_mat.n_cols<<": "<<x_i_mat<<endl;
    return HDP<U>::addDoc(x_i_mat);
  };

  // works on the data in z_i -> size has to be correct in order for this to work!
  // makes a copy of the internal labels vector
  bool getClassLabels(numeric::array& z_i, uint32_t i)
  {
    Col<uint32_t> z_i_col;
    if(!HDP<U>::getClassLabels(z_i_col, i)){return false;} // works on the data in z_i_mat
    Col<uint32_t> z_i_wrap=np2col<uint32_t>(z_i); // can do this since x_i_mat gets copied inside
    if(z_i_col.n_rows != z_i_wrap.n_rows)
      return false;
    else{
      for (uint32_t i=0; i<z_i_wrap.n_rows; ++i)
        z_i_wrap.at(i)=z_i_col.at(i);
      return true;
    }
  };
};

//typedef DP<uint32_t> DP_Dir;
//typedef DP<double> DP_INW;
typedef HDP_py<uint32_t> HDP_Dir;
typedef HDP_py<double> HDP_INW;

class HDP_onl_py : public HDP_onl
{
public:
  HDP_onl_py(const BaseMeasure<uint32_t>& base, double alpha, double gamma)
  : HDP_onl(base,alpha,gamma)
  {
    cout<<"Creating "<<typeid(this).name()<<endl;
  };

  bool densityEst(uint32_t Nw, double kappa, uint32_t K, uint32_t T, uint32_t S)
  {
    cout<<"mX.size()="<<HDP_onl::mX.size()<<endl;
    cout<<"mX_ho.size()="<<HDP_onl::mX_ho.size()<<endl;
//    for (uint32_t i=0; i<HDP_onl::mX.size(); ++i)
//      cout<<"  x_"<<i<<": "<<HDP_onl::mX[i].n_rows<<"x"<<HDP_onl::mX[i].n_cols<<endl;
//
    return HDP_onl::densityEst(Nw,kappa,K,T,S);
  }

  // makes no copy of the external data x_i
  uint32_t addDoc(const numeric::array& x_i)
  {
    Mat<uint32_t> x_i_mat=np2mat<uint32_t>(x_i); // can do this since x_i_mat gets copied inside
    return HDP_onl::addDoc(x_i_mat);
  };

  uint32_t addHeldOut(const numeric::array& x_i)
  {
    Mat<uint32_t> x_i_mat=np2mat<uint32_t>(x_i); // can do this since x_i_mat gets copied inside
    return HDP_onl::addHeldOut(x_i_mat);
  };


  // after an initial densitiy estimate has been made using addDoc() and densityEst()
  // can use this to update the estimate with information from additional x 
  bool updateEst(const numeric::array& x, double ro=0.75)
  {
    Mat<uint32_t> x_mat=np2mat<uint32_t>(x); // can do this since x_mat gets copied inside    
    return HDP_onl::updateEst(x_mat,ro);
  }


  // works on the data in z_i -> size has to be correct in order for this to work!
  // makes a copy of the internal labels vector
  bool getClassLabels(numeric::array& z_i, uint32_t i)
  {
    Col<uint32_t> z_i_col;
    if(!HDP_onl::getClassLabels(z_i_col, i)){return false;} // works on the data in z_i_mat
    Col<uint32_t> z_i_wrap=np2col<uint32_t>(z_i); // can do this since x_i_mat gets copied inside
    if(z_i_col.n_rows != z_i_wrap.n_rows)
      return false;
    else{
      for (uint32_t i=0; i<z_i_wrap.n_rows; ++i)
        z_i_wrap.at(i)=z_i_col.at(i);
      return true;
    }
  };

  // works on the data in lambda -> size has to be correct in order for this to work!
  // makes a copy of the internal labels vector
  bool getLambda(numeric::array& lambda, uint32_t k)
  {
    Col<double> lambda_col;
    if(!HDP_onl::getLambda(lambda_col, k)){return false;} // works on the data in _mat
    Col<double> lambda_wrap=np2col<double>(lambda); 
    if(lambda_col.n_rows != lambda_wrap.n_rows)
      return false;
    else{
      for (uint32_t i=0; i<lambda_wrap.n_rows; ++i)
        lambda_wrap.at(i)=lambda_col.at(i);
      return true;
    }
  };

  void getA(numeric::array& a)
  {
    Col<double> a_wrap=np2col<double>(a); 
     for (uint32_t i=0; i<a_wrap.n_rows; ++i)
        a_wrap.at(i)=HDP_onl::mA.at(0,i);
  };

  void getB(numeric::array& b)
  {
    Col<double> b_wrap=np2col<double>(b); 
     for (uint32_t i=0; i<b_wrap.n_rows; ++i)
        b_wrap.at(i)=HDP_onl::mA.at(1,i);
  };

  void getPerplexity(numeric::array& perp)
  {
    Col<double> perp_wrap=np2col<double>(perp); 
     for (uint32_t i=0; i<perp_wrap.n_rows; ++i)
        perp_wrap.at(i)=HDP_onl::mPerp.at(i);
  };

  bool getDocTopics(numeric::array& pi, numeric::array& prop, numeric::array& topicInd, uint32_t d)
  {
    Col<double> prop_col;
    Col<double> pi_col;
    Col<uint32_t> topicInd_col;
    if(!HDP_onl::getDocTopics(pi_col, prop_col, topicInd_col, d)){return false;} // works on the data in _mat
    Col<double> prop_wrap=np2col<double>(prop); 
    Col<double> pi_wrap=np2col<double>(pi); 
    Col<uint32_t> topicInd_wrap=np2col<uint32_t>(topicInd); 
    if((prop_col.n_rows != prop_wrap.n_rows) || (topicInd_col.n_rows != topicInd_wrap.n_rows) || (pi_col.n_rows != pi_wrap.n_rows))
      return false;
    else{
     for (uint32_t i=0; i<pi_col.n_rows; ++i)
        pi_wrap.at(i)=pi_col.at(i);
     for (uint32_t i=0; i<topicInd_wrap.n_rows; ++i)
        topicInd_wrap.at(i)=topicInd_col.at(i);
      for (uint32_t i=0; i<prop_wrap.n_rows; ++i)
        prop_wrap.at(i)=prop_col.at(i);
      return true;
    }
  };

  bool getCorpTopicProportions(numeric::array& v, numeric::array& sigV)
  {
    Col<double> sigV_col;
    Col<double> v_col;
    if(!HDP_onl::getCorpTopicProportions(v_col,sigV_col)){return false;} // works on the data in _mat
    Col<double> sigV_wrap=np2col<double>(sigV); 
    Col<double> v_wrap=np2col<double>(v); 
    if((sigV_col.n_rows != sigV_wrap.n_rows) || (v_col.n_rows != v_wrap.n_rows))
      return false;
    else{
      for (uint32_t i=0; i<v_wrap.n_rows; ++i)
        v_wrap.at(i)=v_col.at(i);
      for (uint32_t i=0; i<sigV_wrap.n_rows; ++i)
        sigV_wrap.at(i)=sigV_col.at(i);
      return true;
    }
  }; 

  bool getCorpTopic(numeric::array& beta, uint32_t k)
  {
    Col<double> beta_col;
    if(!HDP_onl::getCorpTopic(beta_col, k)){return false;} // works on the data in _mat
    Col<double> beta_wrap=np2col<double>(beta); 
    if(beta_col.n_rows != beta_wrap.n_rows)
      return false;
    else{
      for (uint32_t i=0; i<beta_wrap.n_rows; ++i)
        beta_wrap.at(i)=beta_col.at(i);
      return true;
    }
  };

  bool getWordTopics(numeric::array& z, uint32_t d)
  {
    Col<uint32_t> z_col;
    if(!HDP_onl::getWordTopics(z_col, d)){return false;} // works on the data in _mat
    Col<uint32_t> z_wrap=np2col<uint32_t>(z); 
    if(z_col.n_rows != z_wrap.n_rows)
      return false;
    else{
      for (uint32_t i=0; i<z_wrap.n_rows; ++i)
        z_wrap.at(i)=z_col.at(i);
      return true;
    }
  };

  double perplexity(numeric::array& x, uint32_t d, double kappa)
  {
    Mat<uint32_t> x_mat=np2col<uint32_t>(x); // can do this since x_mat gets copied inside    
    return HDP_onl::perplexity(x_mat,d,kappa);
  };

};

BOOST_PYTHON_MODULE(libbnp)
{
	import_array();
	boost::python::numeric::array::set_module_and_type("numpy", "ndarray");

	class_<Dir_py>("Dir", init<numeric::array>())
			.def(init<Dir_py>());
	class_<InvNormWishart_py>("INW",init<const numeric::array, double,
			const numeric::array, double>())
			.def(init<InvNormWishart_py>());

	//	class_<DP_Dir>("DP_Dir",init<Dir_py,double>());
	//	class_<DP_INW>("DP_INW",init<InvNormWishart_py,double>());

	class_<HDP_Dir>("HDP_Dir",init<Dir_py&,double,double>())
        .def("densityEst",&HDP_Dir::densityEst)
        .def("getClassLabels",&HDP_Dir::getClassLabels)
        .def("addDoc",&HDP_Dir::addDoc);
  //      .def_readonly("mGamma", &HDP_Dir::mGamma);

	class_<HDP_INW>("HDP_INW",init<InvNormWishart_py&,double,double>())
        .def("densityEst",&HDP_INW::densityEst)
        .def("getClassLabels",&HDP_INW::getClassLabels)
        .def("addDoc",&HDP_INW::addDoc);
  //      .def_readonly("mGamma", &HDP_INW::mGamma);

	class_<HDP_onl_py>("HDP_onl",init<Dir_py&,double,double>())
        .def("densityEst",&HDP_onl_py::densityEst)
        .def("updateEst",&HDP_onl_py::updateEst)
        .def("perplexity",&HDP_onl_py::perplexity)
        .def("getClassLabels",&HDP_onl_py::getClassLabels)
        .def("addDoc",&HDP_onl_py::addDoc)
        .def("addHeldOut",&HDP_onl_py::addHeldOut)
        .def("getPerplexity",&HDP_onl_py::getPerplexity)
        .def("getA",&HDP_onl_py::getA)
        .def("getB",&HDP_onl_py::getB)
        .def("getLambda",&HDP_onl_py::getLambda)
        .def("getDocTopics",&HDP_onl_py::getDocTopics)
        .def("getWordTopics",&HDP_onl_py::getWordTopics)
        .def("getCorpTopicProportions",&HDP_onl_py::getCorpTopicProportions)
        .def("getCorpTopic",&HDP_onl_py::getCorpTopic);
   //     .def_readonly("mGamma", &HDP_onl_py::mGamma);

}

