#include <JuceHeader.h>
using namespace juce;
using namespace snex;
using namespace snex::Types;
using namespace scriptnode;
using namespace Interleaver;
static constexpr int NumChannels = 2;
hmath Math;
namespace basic_test{
namespace abs_calls
{

float main(float input)
{
	float z = input - 1.0f;

	// You probably know the Math.abs() 
	// from the HiseScript Math class
	return Math.abs(input) + Math.abs(z);
}

};

}
namespace basic_test{
namespace complex_simple_register
{

struct X
{
    int getX()
    {
        return Math.max(2, x) + 2;
    }
    
    int x = 3;
};

int main(int input)
{
	// Use a initialiser list to overwrite the initial x value
	X obj = { 6 };
	
	return obj.getX();
}

};

}
namespace basic_test{
namespace constant_override
{

struct X
{
    static const int NumChannels = 3;
    
    int getX()
    {
        return NumChannels;
    }
};



int main(int input)
{
	X x;
	
	return NumChannels + x.getX();
}

};

}
namespace basic_test{
namespace function_call
{

int other()
{
    return 2;
}

int main(int input)
{
    return other();
}

};

}
namespace basic_test{
namespace function_call_byvalue
{



struct X
{
    int v = 12;
};

using MySpan = span<X, 2>;

void resetObject(MySpan copy)
{
    copy[0].v = 0;
}

MySpan x;

int main(int input)
{
	resetObject(x);
	
	return x[0].v;
}

};

}
namespace basic_test{
namespace function_overload
{

int get(double input){ return 3;}
int get(float input){ return 9;}
			
int main(int input)
{
	float v = (float)input;
    return get(v);
}

};

}
namespace basic_test{
namespace function_pass_structbyvalue
{

struct X
{
    int v = 110;
    float d = 1.0f;
    float x = 4.0f;
    float y = 8.0f;
    span<int, 40> data = { 90 };
};

int getValueFromX(X copy)
{
    copy.v = 9;
    return copy.data[0];
}

X x;

int main(int input)
{
	return x.v + getValueFromX(x); 
}

};

}
namespace basic_test{
namespace function_ref
{

// The & means reference, so the function
// will operate on the argument that was passed in
void setToFour(int& r)
{
	// the function was called with x,
	// so this will change x
    r = 4;
}

int x = 12;

int main(int input)
{	
	setToFour(x);
	
	return x;
}

};

}
namespace basic_test{
namespace function_ref_local
{

void setToFour(int& r)
{
    r = 4;
}



int main(int input)
{	
    int x = 12;
    
	setToFour(x);
	
	return x;
}

};

}
namespace basic_test{
namespace function_return_ref
{



struct X
{
    span<int, 2> data = { 1, 2 };
    
    int& getData()
    {
        return data[0];
    }
};


X obj;

int main(int input)
{
	auto& x = obj.getData();
	x = 9;
	return obj.data[0];
}

};

}
namespace basic_test{
namespace function_with_same_parameter_name
{

int other(int input)
{
    return input * 2;
}

int main(int input)
{
	return other(input + 3);
}

};

}
namespace basic_test{
namespace function_with_ternary
{

float condGet(float input)
{
    return input > 10.0f ? 90.0f : 40.0f;
}

float main(float input)
{
	return condGet(input);
    
}

};

}
namespace basic_test{
namespace global_variable
{

int x = 9;

int main(int input)
{
    return x;
}

};

}
namespace basic_test{
namespace if_globalwrite
{


float x = 1.0f; 

float main(float input) 
{ 
	x = 1.0f; 
	
	if (input < -0.5f) 
		x = 12.0f; 
		
	return x; 
}};

}
namespace basic_test{
namespace inc_after_cond
{

int count = 12;

int main(int input)
{
	if(count >= 12)
	{
		count = 0;
	}
	
  count++;

	return count;
}

};

}
namespace basic_test{
namespace inc_as_var
{

int x = 5;

int main(int input)
{
	int c = ++x;
	
	return x;
}

};

}
namespace basic_test{
namespace local_override
{

int y = 1;

int main(int input)
{
	int x = y;
	return x;
}

};

}
namespace basic_test{
namespace local_span_ref
{



int main(int input)
{
	span<int, 9> d = { 80};

	auto& x = d[2];
	
	d[2] = 9;
	
	return d[2];
}

};

}
namespace basic_test{
namespace nested_function_call
{

int op1(int in)
{
	return in * 5;
}

int op2(int in2)
{
	return in2 + 1;
}

int f = 3;

int main(int input)
{
	return op2(op1(f));
}

};

}
namespace basic_test{
namespace nested_struct_member_call
{

struct X 
{ 
    struct Y 
    { 
        int u = 8; 
        float v = 12.0f; 
        
        float getV() 
        { 
            return v; 
        }
    }; 
    Y y; 
}; 

X x; 

int main(int input)
{
	return 12;
	//return x.y.getV() + input;
}

};

}
namespace basic_test{
namespace pass_struct_ref
{

void init(OscProcessData& d)
{
	d.uptime = 5.0;
}



int main(int input)
{
	OscProcessData f;
	
	init(f);
	
	return (int)f.uptime;
}

};

}
namespace basic_test{
namespace post_inc
{

int x = 9;

int main(int input)
{
    return x++;
}

};

}
namespace basic_test{
namespace pre_inc
{

int x = 9;

int main(int input)
{
    return ++x;
}

};

}
namespace basic_test{
namespace reuse_span_register
{

span<float, 5> data = { 2.0f };



float main(float input)
{
    data[0] = 12.0f;
    data[0] = 13.0f;
    
	  return data[0];
}};

}
namespace basic_test{
namespace reuse_struct_member_register
{

struct X
{
    int v = 0;
};

X x;

int main(int input)
{
    x.v = 13;
	x.v = 12;
	return x.v;
}

};

}
namespace basic_test{
namespace scoped_local_override
{

int x = 1;

int main(int input)
{
    {
	    int x = x;
	    x = 9;
    }
    
	return x;
}

};

}
namespace basic_test{
namespace set_from_other_function
{

span<int, 2> d = { 1, 5 };
int x = 9;

int main(int input)
{
    return d[0] + d[1];
}

};

}
namespace basic_test{
namespace simplecast
{

int main(float input)
{
	return (int)input;
}

};

}
namespace basic_test{
namespace simplereturn
{

int main(int input)
{
	return 12;
}

};

}
namespace basic_test{
namespace simplestruct
{

struct MyObject
{
    int x = 9;
    int y = 12;
};

MyObject c;

int main(int input)
{
	return c.x + c.y;
}

};

}
namespace basic_test{
namespace simple_if
{

int main(int input)
{
    if(input > 8)
        return 5;
        
    return 9;
}

};

}
namespace basic_test{
namespace span_iteration
{

span<float, 16> c1 = { 4.0f };

int main(int input)
{
    for(auto& s: c1)
    {
        s *= 0.5f;
    }
    
    return (int)c1[0];
}

};

}
namespace basic_test{
namespace static_function_call
{

static int getStaticInRoot()
{
	return 8;
}

struct X
{
	static const int z = 12;

	static int getStaticValue()
	{
		return z + 5;
	}
};


int main(int input)
{
	return X::getStaticValue() + getStaticInRoot();
}

};

}
namespace basic_test{
namespace static_function_call2
{



span<int, 2> data = { 4 };

static void set(span<int, 2>& d)
  {
    d[0] = 8;
  }

struct X
{
	
};

int main(int input)
{
	set(data);
	return data[0];
}

};

}
namespace basic_test{
namespace ternary
{

int main(int input)
{
	return input > 8 ? 5 : 9;
}

};

}
namespace default_arg_test{
namespace default_1
{

int getDefault(int x=4)
{
	return x * 2;
}

int main(int input)
{
	return getDefault();
}

};

}
namespace default_arg_test{
namespace default_2
{

float get5()
{
	return 5.0f;
}

int getDefault(int x=(int)get5())
{
	return x * 2;
}

int main(int input)
{
	return getDefault();
}

};

}
namespace default_arg_test{
namespace default_4
{


span<int, 10> data = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };

dyn<int> data1;
dyn<int> data2;
dyn<int> data3;



int main(int input)
{
	data1.referTo(data);
	data2.referTo(data, 3);
	data3.referTo(data, 4, 2);
	
	return data1.size() + data2.size() + data3.size();
}

};

}
namespace default_arg_test{
namespace dyn_size_defaultarg
{


span<int, 10> data = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };

dyn<int> data1;
dyn<int> data2;
dyn<int> data3;



int main(int input)
{
	data1.referTo(data);
	data2.referTo(data, 3);
	data3.referTo(data, 4, 2);
	
	auto sizeSum = data1.size() + data2.size() + data3.size();
	auto firstSum = data1[0] + data2[0] + data3[0];
	
	return sizeSum + firstSum;
}

};

}
namespace default_arg_test{
namespace IndexTypeDefault
{


span<int, 10> data = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };


int main(int input)
{
  auto firstIndex = IndexType::wrapped(data);

	auto first = data[firstIndex];
	
  auto secondIndex = IndexType::wrapped(data, input);

	auto second = data[secondIndex];
	
	return first + second;
	
}

};

}
namespace dsp_test{
namespace delay_funk
{

using DelayBuffer = span<float, 1024>;
using WrappedIndex = DelayBuffer::wrapped;

span<DelayBuffer, NumChannels> buffer;
WrappedIndex readBuffer;
WrappedIndex writeBuffer;

int main(int input)
{
    
    
	auto& l = buffer[0];
	
	l[1] = 5.0f;
	
	return buffer[0][1];
}

};

}
namespace dsp_test{
namespace sine_class
{




int main(int input)
{
	
	return 12;
}

};

}
namespace dsp_test{
namespace slice_1
{

span<float, 8> data = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f };

int main(int input)
{
	auto d = slice(data, 4, 4);
	
	for(auto& v: d)
	    v = 9.0f;
	    
	return (int)data[6];
}

};

}
namespace dsp_test{
namespace slice_2
{

span<float, 8> data = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f };

int main(int input)
{
	auto d = slice(data, 4, 4);
	
	for(auto& v: slice(d, 2, 2))
	    v = 9.0f;
	    
	return (int)data[6];
}

};

}
namespace dsp_test{
namespace slice_3
{

span<float, 8> data = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f };

int main(int input)
{
	auto d = slice(data, 4, 4);
	
	for(auto& v: slice(d, 2, 2))
	    v = 9.0f;
	    
	return (int)data[6];
}

};

}
namespace dsp_test{
namespace slice_4
{

using DynWrapType = dyn<float>::unsafe;

span<float, 8> data = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f };



int main(int input)
{
	auto d = slice(data, 8, 0);
	
	int numElements = 0;
	
	for(auto& s: d)
    {
        numElements++;
    }
	    
	return numElements;
}

};

}
namespace dsp_test{
namespace slice_5
{

span<float, 8> data = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f };

using DynWrapType = dyn<float>::unsafe;

int main(int input)
{
	auto d = slice(data, 8, 0);
	auto d2 = slice(d, 8, 0);
	
	int numElements = 0;
	
	for(auto& s: d2)
    {
        s = 9.0f;
    }
	    
	return (int)data[5];
}

};

}
namespace dsp_test{
namespace slice_6
{

span<float, 8> data = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f };

using DynWrapType = dyn<float>::unsafe;

int main(int input)
{
	auto d = slice(data, 1, 4);
	
	for(auto& s: d)
    {
        s = 9.0f;
    }
	    
	return (int)data[4];
}

};

}
namespace dsp_test{
namespace slice_pretest
{

int x = 19;

int main(int input)
{
	return Math.range(input, 2, x);
}

};

}
namespace dyn_test{
namespace dyn2simd
{

span<float, 8> s = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f };
dyn<float> d;


int main(int input)
{
	d.referTo(s);
	float sum = 0.0f;
	
	for(auto& f4: d.toSimd())
    {
        sum += f4[0];
        sum += f4[1];
        sum += f4[2];
        sum += f4[3];
    }
    
    return sum;
}

};

}
namespace dyn_test{
namespace dyn_access1
{

span<int, 4> data = { 1, 2, 3, 4 };
dyn<int> b;

int main(int input)
{
	b.referTo(data);
	
	return b[1];
}

};

}
namespace dyn_test{
namespace dyn_access2
{

span<int, 10> data = { 4 };
dyn<int> b;

int main(int input)
{
	b.referTo(data);
	
	return b[29 % input];
}

};

}
namespace dyn_test{
namespace dyn_subscript
{

span<int, 5> c = { 1, 2, 3, 4, 5 };
dyn<int> x;

int main(int input)
{
  dyn<int>::wrapped i = 2;
	x.referTo(c);

  return x[i];
}

};

}
namespace dyn_test{
namespace dyn_wrap_3
{

span<float, 6> s = { 1.0f, 2.0f, 3.0f, 4.f, 5.0f, 6.f };
dyn<float> d;

dyn<float>::unsafe i;

void assign()
{
    d.referTo(s);
}

float main(float input)
{
    assign();
    
    return d[i] + d[i.moved(1)];
}

};

}
namespace dyn_test{
namespace global_dyn_with_assignment
{

span<int, 8> data = { 2 };

dyn<int> d;

int main(int input)
{
    d.referTo(data);
	
    for(auto& s: d)
    {
        input += s;
    }
    
    return input;
}

};

}
namespace dyn_test{
namespace simple_dyn_test
{

span<int, 8> data = { 2 };

int main(int input)
{
    dyn<int> d;

    d.referTo(data);
	
    for(auto& s: d)
    {
        input += s;
    }
    
    return input;
}

};

}
namespace dyn_test{
namespace size_test
{

span<float, 19> d;

dyn<float> data;

int main(int input)
{
	data.referTo(d);
	
	return data.size();
}

};

}
namespace dyn_test{
namespace span_of_dyns
{

span<int, 8> data = { 2 };

span<int, 8> data2 = { 4 };

using StereoChannels = span<dyn<int>, 2>;

StereoChannels d;

int main(int input)
{
    d[0].referTo(data);
    d[1].referTo(data2);
	
    for(auto& s: d)
    {
        for(auto& v: s)
            input += v;
    }
    
    return input;
}

};

}
namespace enum_test{
namespace enum_as_template_argument
{

enum class MyEnum
{
	First = 9
};

template <int N> int getArgs()
{
	return N;
}

int main(int input)
{
	return getArgs<(int)MyEnum::First>();
}

};

}
namespace enum_test{
namespace enum_in_struct
{


struct X
{
	enum MyEnum
	{
		First = 9,
		Second,
		Third = 20
	};
};

int main(int input)
{
	return X::MyEnum::First + X::Third;
}

};

}
namespace enum_test{
namespace enum_set_parameter
{

struct processor
{
	enum Parameter
	{
		FirstParameter,
		SecondParameter,
		numParameters
	};
	
	template <int P> void setParameter(double value)
	{
		v[P] = value * 2.0;
	}
	
	span<double, Parameter::numParameters> v;
};

processor p;

void change(double input)
{
	p.setParameter<1>(input);	
}

double main(double input)
{
	change(input);
	
	
	return p.v[1];
}

};

}
namespace enum_test{
namespace simple_enum
{

enum class MyEnum
{
	First = 0,
	Second,
	numItems
};

int main(int input)
{
	return (int)MyEnum::First + (int)MyEnum::Second;
}

};

}
namespace expression_initialiser_test{
namespace expression_initialiser
{

int x = 6;
float v = 9.0f;

struct X
{
    int value = 0;
    double z = 0.0f;
};

int main(int input)
{
  X obj = { x+8, Math.abs((double)input) };
  
  return obj.value + (int)obj.z;
}

};

}
namespace expression_initialiser_test{
namespace expression_initialiser_nested
{

struct X
{
    int x = 6;
    
    struct Y
    {
        double d1 = 12.0;
        double d2 = 2.0;
        
        double sum()
        {
            return d1 + d2;
        }
    };
    
    Y y1;
    
    float z = 25.0f;
};

double d = 7.0;

int main(int input)
{
	X obj = { 9, { d, d + 3.0 }, 4.0f };
	
	return obj.x + (int)(obj.y1.sum()) + (int)obj.z;
}

};

}
namespace expression_initialiser_test{
namespace expression_initialiser_span
{

struct X
{
    int sum()
    {
        return v1 + (int)f1;
    }
    
    int v1 = 0;
    float f1 = 0.0f;
};

int i = 1;
float f = 4.0f;

int main(int input)
{
	span<X, 2> x = { {i, 2.0f }, {i+2, f } };
	
	return x[0].sum() + x[1].sum();
}

};

}
namespace expression_initialiser_test{
namespace expression_initialiser_span_single
{

int v = 12;

int main(int input)
{
	span<int, 2> x = { ++v };
	
	return x[0] + x[1];
}

};

}
namespace expression_initialiser_test{
namespace expression_list_span2dyn
{

span<float, 8> s = { 1.0f };

int main(int input)
{
	dyn<float> d = { s };
	
	int x = 0;
	
	for(auto& sample: d)
    {
        x += (int)sample;
    }
    
    return x;
}

};

}
namespace float4_test{
namespace float4_basic01
{

float4 d = { 19.0f };

float main(float input)
{
	return d[0];
	
}

};

}
namespace float4_test{
namespace float4_basic02
{

float4 d = { 19.0f };

float main(float input)
{
	auto e = d;
	e[0] = 90.0f;	

	return d[0];
	
}

};

}
namespace float4_test{
namespace float4_basic03
{

span<float, 4> d = { 19.0f };

float main(float input)
{
	d[0] = 90.0f;	

	return d[0];	
}

};

}
namespace init_test{
namespace init_test1
{


struct X
{
	X()
	{
		value = 12;
	}

	int value = 9;
};

X obj;

int main(int input)
{
	return obj.value;
}};

}
namespace init_test{
namespace init_test10
{

struct X
{
	X()
	{
		// this value will override the initialiser list
		value = 4;
	}
	
	int value = 9;
};

span<span<X, 2>, 2> data;

int main(int input)
{
	return data[0][0].value + data[1][0].value;
}

};

}
namespace init_test{
namespace init_test11
{

int counter = 0;

struct X
{
	X()
	{
		value = 2;
	
		counter++;
	}
	
	int value = 9;
};



int main(int input)
{
	span<span<X, 6>, 2> data;
	return counter;
}

};

}
namespace init_test{
namespace init_test12
{

int counter = 0;

struct X
{
	struct Y
	{
		Y()
		{
			value = 12;
		}
		
		int value = 0;
	};
	
	Y y;
};

X obj;

int main(int input)
{
	return obj.y.value;
}

};

}
namespace init_test{
namespace init_test13
{

int counter = 0;

struct X
{
	X()
	{
		value = 1;
	}
	
	int value = 0;
};


int main(int input)
{
	X obj, obj2;
	
	return obj.value + obj2.value;
}

};

}
namespace init_test{
namespace init_test14
{

struct X
{
	X(int v)
	{
		value = v;
	}
	
	int value = 0;
};

int v = 91;

int main(int input)
{
	X obj = { v+12 };
	
	return obj.value;
}

};

}
namespace init_test{
namespace init_test15
{

struct X
{
	struct Y
	{
		Y()
		{
			value = 103;
		}
		
		int value = 0;
		int value2 = 1000;
	};
	
	struct Z
	{
		Z()
		{
			z = 90.0;
		}
		
	
		double z = 0.0;
	};
	
	Y y;
	Y y2;
	Z z;
};


int main(int input)
{
	X obj;
	
	return obj.y.value + obj.y2.value + (int)obj.z.z + obj.y.value2;
}
};

}
namespace init_test{
namespace init_test16
{

struct X
{
	int value = 5;
};

struct Y
{
	Y(X& x)
	{
		x.value = 90;
	}
};


int main(int input)
{
	X obj;
	
	Y y = { obj };
	
	return obj.value;
}
};

}
namespace init_test{
namespace init_test17
{

struct X
{
	DECLARE_NODE(X);

	template <int P> void setParameter(double v) {}

	void reset()
	{
		value *= 2;
	}
	

	int value = 8;
};

struct I
{
	I(X& x)
	{
		x.value = 12;
	}
};


wrap::init<X, I> obj;

int main(int input)
{
 
 	obj.reset();
  	
	return obj.obj.value;
	
	
}

};

}
namespace init_test{
namespace init_test18
{

struct X
{
	DECLARE_NODE(X);

	void reset()
	{
		value *= 2;
	}
	
	template <int P> void setParameter(double v)
	{
		if(P == 0)
			value = (int)v;
	}
	

	int value = 8;
};

struct I
{
	I(X& x)
	{
		x.value = 12;
	}
};

struct X2
{
	DECLARE_NODE(X2);

	template <int P> void setParameter(double newValue)
	{
		v = (int)newValue;
	}
	

	int v = 9;
};

struct I2
{
	I2(X2& x2)
	{
		x2.v = 80;
	}
};


wrap::init<X, I> o;
wrap::init<X2, I2> o2;

X before;

int main(int input)
{
	//before.setParameter<0>(90.0);

  	o.setParameter<0>(20.0);
  	o2.setParameter<0>(1000.0);
  	
	return o.obj.value + o2.obj.v;
}
};

}
namespace init_test{
namespace init_test19
{

struct X
{
	template <int P> void setParameter(double v)
	{
		value = v;
	}
	
	double value = 0.0;
};


template <typename T> struct Wrapper
{
	template <int P2> void setParameter(double v)
	{
		obj.setParameter<P2>(v);
	}
	

	T obj;
};


Wrapper<X> w;

int main(int input)
{
	w.setParameter<20>(20.0);
	
	

	return (int)w.obj.value;
}};

}
namespace init_test{
namespace init_test2
{


struct X
{
	void init()
	{
		value = 12;
	}
	

	X()
	{
		init();
	}

	int value = 9;
};

X obj;

int main(int input)
{
	return obj.value;
}};

}
namespace init_test{
namespace init_test20
{

struct X
{
	X()
	{
		constructedValue = 5;
	}
	
	int constructedValue = 1;
	int initValue = 4;
};


X obj;

int main(int input)
{
	X obj2;
	
	return obj.constructedValue + obj.initValue +
	       obj2.constructedValue + obj2.initValue;
}


};

}
namespace init_test{
namespace init_test21
{

struct e 
{
	DECLARE_NODE(e);

	template <int P> void setParameter(double v) {}

	void reset()
	{
		
	}
	
	int v = 90;
};


struct i
{
	i(wrap::event<e>& o)
	{
		o.getWrappedObject().v = 18;
	}
};

wrap::init<wrap::event<e>, i> obj;

int main(int input)
{
	return obj.getWrappedObject().v;
}

};

}
namespace init_test{
namespace init_test22
{

struct e 
{
	DECLARE_NODE(e);
	static const int NumChannels = 1;

	template <int P> void setParameter(double v)
	{
		
	}

	void reset()
	{
		
	}
	
	int v = 90;
};


struct i
{
	i(wrap::event<e>& o)
	{
		o.getObject().v = 18;
	}
};



container::chain<parameter::empty, wrap::fix<1, wrap::init<wrap::event<e>, i>>> obj3;

int main(int input)
{
	return obj3.get<0>().getWrappedObject().v;
}
};

}
namespace init_test{
namespace init_test23
{

struct MyObject
{
	MyObject(int initValue)
	{
		v = initValue;
	}
	
	int v = 40;
};




int main(int input)
{
	auto o3 = MyObject(90);
		
	
	MyObject o2(input);
	
	
	MyObject o1 = { input * 2};
	
	return o3.v + o2.v + o1.v;
}

};

}
namespace init_test{
namespace init_test24
{

struct MyObject
{
	MyObject()
	{
		v = 70;
	}
	
	int v = 40;
};

struct Nested
{
	Nested()
	{
		o1.v = 90;
		o2.v = 80;
	}
	
	int get() const
	{
		return o1.v + o2.v;
	}

	MyObject o1;
	MyObject o2;
};


Nested obj;

int main(int input)
{
	Nested obj2;
	
	return obj.get() + obj2.get();
}

};

}
namespace init_test{
namespace init_test3
{


struct X
{
	X(int v)
	{
		value = v * 2;
	}

	int value = 9;
};

X obj = { 4 };

int main(int input)
{
	return obj.value;
}};

}
namespace init_test{
namespace init_test4
{

struct Y
{
	int yValue = 90;
};


struct X
{
	X(Y& o)
	{
		value = o.yValue * 2;
	}

	int value = 9;
};

Y yObj;
X obj = { yObj };

int main(int input)
{
	return obj.value;
}};

}
namespace init_test{
namespace init_test5
{

struct Y
{
	int yValue = 90;
};


struct X
{
	X(int o)
	{
		value = o * 2;
	}

	int value = 9;
};

Y yObj;
X obj = { yObj.yValue };

int main(int input)
{
	return obj.value;
}};

}
namespace init_test{
namespace init_test6
{

struct X
{
	X(int a, float z, double b)
	{
		v1 = a;
		v2 = z;
		v3 = b;
	}
	
	int v1 = 0;
	float v2 = 0.0f;
	double v3 = 0.0;
};

X obj = { 1, 2.0f, 3.0};


int main(int input)
{
	return (int)obj.v3 + (int)obj.v2 + obj.v1;
}

};

}
namespace init_test{
namespace init_test7
{


namespace X
{

	struct Inner
	{
		Inner(int z)
		{
			value = z;
		}
	
	
		int value = 0;
	};
}

X::Inner obj = { 2 };


int main(int input)
{
	return obj.value;
}

};

}
namespace init_test{
namespace init_test8
{

struct X
{
	X()
	{
		value = 6;
	}
	
	int value = 9;
};

span<X, 2> data;

int main(int input)
{
	return data[0].value + data[1].value;
}

};

}
namespace init_test{
namespace init_test9
{

struct X
{
	X() {}
	
	int value = 9;
};

span<X, 2> data;

int main(int input)
{
	return data[0].value + data[1].value;
}

};

}
namespace loop_test{
namespace dyn_simd_1
{

span<float, 15> s = { 1.0f };

dyn<float> d;

int main(float input)
{
	d.referTo(s);

	for(auto& v: d)
    {
        v = 8.0f;
    }
	
	for(auto& v: d)
    {
        input += v;
    }
    
    auto result = s.size() * 8;
    
    return  result == input;
}

};

}
namespace loop_test{
namespace loop2simd_1
{

span<float, 8> data = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f };

int main(int input)
{
	for(auto& s: data)
    {
        s += 2.0f;
    }
    
    float x = 0.0f;
    
    for(auto& s: data)
    {
        x += s;
    }
    
    
	return (int)x;
}

};

}
namespace loop_test{
namespace loop2simd_2
{

span<float, 8> data = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f };

int main(int input)
{
    
    float y = 1.0f;
    
	for(auto& s: data)
    {
        s += y;
        
    }
    
    float x = 0.0f;
    
    for(auto& s: data)
    {
        x += s;
    }
    
	return (int)x;
}

};

}
namespace loop_test{
namespace loop2simd_3
{

span<float, 8> data = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f};

int main(int input)
{
    
	for(auto& s: data)
    {
        s += 2.0f;
    }
    
   
   for(auto& s: data)
   {
      input += s;
   }
    
	return input;
}

};

}
namespace loop_test{
namespace loop2simd_4
{


span<float, 8> data = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f};

int main(int input)
{
  
    auto& x = data.toSimd();
    
	 x[0] += 21.0f;
    
	//Console.dump();
   
    
	return data[0];
}

};

}
namespace loop_test{
namespace loop_combine1
{

span<float, 37> data = { 0.0f };

int main(int input)
{
	for(auto& s: data)
	    s += 0.4f;
	    
	for(auto& s: data)
	    s *= 0.5f;
	    
	return (int)(data[0] * 10.0f);
}

};

}
namespace loop_test{
namespace loop_combine2
{

span<float, 37> data = { 0.0f };


void add()
{
	for(auto& s: data)
	    s += 0.4f;
}

void mul()
{
	for(auto& s: data)
	s *= 0.5f;
}

int main(int input)
{
	add();
	mul();
	    
	
	    
	return (int)(data[0] * 10.0f);
}

};

}
namespace loop_test{
namespace loop_combine3
{

span<float, 37> data = { 0.0f };

int main(int input)
{
	for(auto& s: data)
	    s += 0.4f;
	    
	for(auto& s: data)
	    s *= 0.5f;
	    
	for(auto& s: data)
	   s /= 0.8f;
	    
	return (int)(data[0] * 10.0f);
}

};

}
namespace loop_test{
namespace loop_combine4
{

span<span<float, 9>, 11> data = { {0.0f} };

int main(int input)
{
	for(auto& m: data)
	{
		for(auto& s: m)
		    s += 0.5f;
	}
	
	for(auto& m: data)
	{
		for(auto& s: m)
		    s *= 0.5f;
	}
	
	return (int)(data[0][0] * 8.0f);
}

};

}
namespace loop_test{
namespace loop_combined9
{

void tut1(span<float, 9>& d)
{
	for(auto& s: d)
	    s += 8.0f;
}

void tut2(span<float, 9>& d)
{
	for(auto& s: d)
	    s *= 8.0f;
}

int main(int input)
{
	span<float, 9> data = { 2.0f };
	
	tut1(data);
	tut2(data);
	
	return (int)data[0];
}

};

}
namespace loop_test{
namespace simdable_non_aligned
{

span<float, 16> s = { 2.0f };
dyn<float> d;

int main(int input)
{
	d.referTo(s, 2, 7);
	
	return d.isSimdable();
}

};

}
namespace loop_test{
namespace unroll_1
{

span<int, 5> d = { 1, 2, 3, 4, 5 };

int main(int input)
{
	for(auto& s: d)
    {
        input += s;
    }
    
    return input;
}

};

}
namespace loop_test{
namespace unroll_2
{

struct X
{
    int v1 = 12;
    float v2 = 18.0f;
    
    float sum()
    {
        return v2 + (float)v1;
    }
};

span<X, 5> d;

int main(int input)
{
    for(auto& s: d)
    {
        input += (int)s.sum();
    }
    
    return input;
}

};

}
namespace loop_test{
namespace unroll_3
{

span<span<int, 2>, 2> d = { {1, 2}, {3, 4} };

int main(int input)
{
	for(auto& l1: d)
    {
        for(auto& l2: l1)
        {
            input += l2;
        }
    }
    
    return input;
}

};

}
namespace loop_test{
namespace unroll_4
{

struct X
{
    double unused = 2.0;
    int v = 5;
};

X x1;
X x2;

span<X, 5> d;

int main(int input)
{
	for(auto& s: d)
	    input += s.v;
	    
    return input;
}

};

}
namespace loop_test{
namespace unroll_5
{

struct X
{
    int v1 = 12;
    float v2 = 18.0f;
};

span<X, 5> d;

int main(int input)
{
    auto& s = d[0];
    input += (int)(s.v2 + (float)s.v1);
    
    return input;
}

};

}
namespace loop_test{
namespace while1
{

int below(int x)
{
  return x < 10;
}

int main(int input)
{
    while(below(input))
    {
        input += 4;
    }
    
    return input;
}

};

}
namespace namespace_test{
namespace empty_namespace
{

namespace Empty
{
    namespace Sub
    {
        
    }
}

int z = 12;

int main(int input)
{
	return z;
}

};

}
namespace namespace_test{
namespace namespaced_struct
{

namespace MyObjects
{
    struct Obj
    {
        int v = 15;
    };
}


MyObjects::Obj c;

int main(int input)
{
    MyObjects::Obj d;
    
	return c.v + d.v;
}

};

}
namespace namespace_test{
namespace namespaced_var
{


namespace Space
{
    int x = 5;
}

int main(int input)
{
	return Space::x;
}

};

}
namespace namespace_test{
namespace nested_namespace
{


namespace Types
{
    namespace RealTypes
    {
        using IntegerType = int;    
    }
}

Types::RealTypes::IntegerType main(Types::RealTypes::IntegerType input)
{
	return input;
}

};

}
namespace namespace_test{
namespace other_namespace
{


namespace N1
{
    using Type = int;
}

namespace N2
{
    N1::Type x = 12;
    using OtherType = N1::Type;
}

N2::OtherType x = 19;

int main(int input)
{
	return N2::x + x;
}

};

}
namespace namespace_test{
namespace simple_namespace
{


namespace Types
{
    using IntegerType = int;
}

Types::IntegerType main(Types::IntegerType input)
{
	return input;
}

};

}
namespace namespace_test{
namespace static_struct_member
{


namespace N1
{
    struct X
    {
        static const int v = 12;
    };
}


int main(int input)
{
	return N1::X::v;
}

};

}
namespace namespace_test{
namespace struct_with_same_name
{


namespace N1
{
    struct X
    {
        int x = 15;  
    };
}

namespace N2
{
    struct X
    {
        int x = 8;
    };
}

N1::X x1;
N2::X x2;


int main(int input)
{
	return x1.x + x2.x;
}

};

}
namespace namespace_test{
namespace struct_with_same_name2
{


namespace N1
{
    struct X
    {
        int x = 15;  
    };
}

namespace N2
{
    struct X
    {
        int x = 8;
    };
}

using namespace N1;

X x1;
N2::X x2;


int main(int input)
{
	return x1.x + x2.x;
}

};

}
namespace namespace_test{
namespace using_namespace
{


namespace N1
{
    using Type = int;
}

namespace N2
{
	using namespace N1;
	
    Type x = 13;
}



int main(int input)
{
    using namespace N2;
	return x;
}

};

}
namespace node_test{
namespace chain_num_channels
{

struct dc
{
	DECLARE_NODE(dc);

	static const int NumChannels = 1;

	template <int P> void setParameter(double v)
	{
		
	}

	void processFrame(span<float, NumChannels>& data)
	{
		data[0] = 1.0f;
	}
	
	
};

container::chain<parameter::empty, dc, dc> c;

int main(int input)
{
	span<float, 1> data = {0.0f};
	
	c.processFrame(data);
	
	return (int)data[0];
	
}

};

}
namespace parameter_test{
namespace parameter_chain
{

struct Identity
{
	static double to0To1(double v)
	{
		return v;
	}
};

struct Test
{
	DECLARE_NODE(Test);

	template <int P> void setParameter(double v)
	{
		value = v;
	}
	
	double value = 12.0;
};

using ParameterType = parameter::plain<Test, 0>;
using ParameterChainType = parameter::chain<Identity, ParameterType, ParameterType>;

ParameterChainType pc;

container::chain<ParameterChainType, wrap::fix<1, Test>, Test> c;

void op()
{
	pc.call(2.0);
}

double main(double input)
{
	auto& first = c.get<0>();
	auto& second = c.get<1>();
	
	pc.connect<0>(first);
	pc.connect<1>(second);

	op();
	
	return c.get<0>().value + c.get<1>().value;
}


};

}
namespace parameter_test{
namespace parameter_expression
{



DECLARE_PARAMETER_EXPRESSION(TestExpression, 2.0 * input - 1.0);


struct Test 
{
	DECLARE_NODE(Test);

	template <int P> void setParameter(double v)
	{
		value = v;
	}
	
	double value = 2.0;
};

using ParameterType = parameter::expression<Test, 0, TestExpression>;

container::chain<ParameterType, wrap::fix<1, Test>, Test> c;

ParameterType p;


double x = 5.0;

double main(double in)
{
	auto& second = c.get<1>();
	p.connect<0>(second);
	
	p.call(x);
	
	return c.get<1>().value;
}

};

}
namespace parameter_test{
namespace parameter_list
{

struct Test
{
	DECLARE_NODE(Test);

	template <int P> void setParameter(double v)
	{
		value = v;
	}
	
	double value = 12.0;
};

using ParameterType = parameter::plain<Test, 0>;
using PList = parameter::list<ParameterType, ParameterType>;

container::chain<PList, wrap::fix<1, Test>, Test> c;

double main(double input)
{
	auto& first = c.get<0>();
	auto& second = c.get<1>();
	
	auto& p1 = c.getParameter<0>();
	auto& p2 = c.getParameter<1>();

	p1.connect<0>(first);
	p2.connect<0>(second);

	c.setParameter<0>(4.0);
	c.setParameter<1>(3.0);

	return c.get<0>().value + c.get<1>().value;
}


};

}
namespace parameter_test{
namespace parameter_mixed
{

struct MyRangeConverter
{
	static double to0To1(double input)
	{
		return input * 4.0;
	}
};

struct OtherTest
{
	DECLARE_NODE(OtherTest);

	template <int P> void setParameter(double v)
	{
		if(P == 1)
			o = v + 10.0;
	}
	
	double o = 100.0;
};

struct Test
{
	DECLARE_NODE(Test);

	template <int P> void setParameter(double v)
	{
		value = v;
	}
	
	double value = 100.0;
};


DECLARE_PARAMETER_EXPRESSION(TestExpression, input * 2.0);

using ParameterType1 = parameter::plain<Test, 0>;
using OtherParameter = parameter::expression<OtherTest, 1, TestExpression>;
using ParameterChainType = parameter::chain<MyRangeConverter, ParameterType1, OtherParameter>;


container::chain<ParameterChainType, wrap::fix<1, Test>, OtherTest> c;

double main(double input)
{
	auto& first = c.get<0>();
	auto& second = c.get<1>();
	
	auto& pc = c.getParameter<0>();

	pc.connect<0>(first);
	pc.connect<1>(second);

	c.setParameter<0>(0.5);

	return c.get<0>().value + c.get<1>().o;
}


};

}
namespace parameter_test{
namespace parameter_range
{





DECLARE_PARAMETER_RANGE(TestRange, 100.0, 120.0);



struct Identity
{
	static double to0To1(double input) { return input; }
};

struct Test 
{
	DECLARE_NODE(Test);

	template <int P> void setParameter(double v)
	{
		if(P == 0)
			value = v;
	}
	
	double value = 1.0;
};



using PType = parameter::from0To1<Test, 0, TestRange>;


container::chain<PType, wrap::fix<1, Test>, Test> c;



double main(double input)
{
	auto& first = c.get<0>();
	
	auto& p = c.getParameter<0>();

	p.connect<0>(first);
	
	c.setParameter<0>(0.5);
	
	return first.value;
}

};

}
namespace parameter_test{
namespace plain_parameter
{

struct Test
{
	DECLARE_NODE(Test);

	template <int P> void setParameter(double v)
	{
		value = v;
	}
	
	double value = 12.0;
};

using ParameterType = parameter::plain<Test, 0>;

container::chain<ParameterType, wrap::fix<1, Test>, Test> c;

double main(double input)
{
	Test obj;
	ParameterType pType;
	pType.connect<0>(obj);
	pType.call(80.0);

	
	auto& first = c.get<0>();
	auto& p = c.getParameter<0>();

	p.connect<0>(first);
	c.setParameter<0>(2.0);

	return c.get<0>().value + obj.value;

}


};

}
namespace parameter_test{
namespace plain_parameter_in_chain
{

struct Test
{
	DECLARE_NODE(Test);

	template <int P> void setParameter(double v)
	{
		value = v;
	}
	
	double value = 12.0;
};

using ParameterType = parameter::plain<Test, 0>;

container::chain<ParameterType, wrap::fix<1, Test>, Test> c;

void op(double input)
{
	c.setParameter<0>(input);	
}

double main(double input)
{
	auto& first = c.get<1>();
	
	auto& p1 = c.getParameter<0>();

	p1.connect<0>(first);

	op(input);
	
	
	return c.get<1>().value;
}


};

}
namespace parameter_test{
namespace plain_parameter_wrapped
{

struct Test
{
	DECLARE_NODE(Test);

	template <int P> void setParameter(double v)
	{
		value = v;
	}
	
	double value = 12.0;
};

using WrappedTest = wrap::event<Test>;

using ParameterType = parameter::plain<Test, 0>;
using WrappedParameterType = parameter::plain<WrappedTest, 0>;

container::chain<WrappedParameterType, wrap::fix<1, WrappedTest>> c2;
container::chain<ParameterType, wrap::fix<1, WrappedTest>, WrappedTest> c;

double main(double input)
{
	c.getParameter<0>().connect<0>(c.get<0>());
	c.setParameter<0>(2.0);

	c2.getParameter<0>().connect<0>(c2.get<0>());
	c2.setParameter<0>(4.0);


	return c.get<0>().value + c2.get<0>().value;

}


};

}
namespace parameter_test{
namespace range_macros
{

DECLARE_PARAMETER_RANGE(TestRange, 100.0, 120.0);


double main(double input)
{
	return TestRange::from0To1(0.5);
}

};

}
namespace preprocessor_test{
namespace constant_and_macro
{

#define VALUE 2
#define MUL(x) x * VALUE

int main(int input)
{
	return MUL(input);
}

};

}
namespace preprocessor_test{
namespace macro_define_type
{

#define MyType int

int main(MyType input)
{
	
	return input / 2;
}

};

}
namespace preprocessor_test{
namespace macro_struct
{

#define VALUE_CLASS(i) struct i { int value = 3; }



VALUE_CLASS(Object);


int main(int input)
{
	Object x;
	
	return x.value;
}

};

}
namespace preprocessor_test{
namespace macro_struct_type
{

#define VALUE_CLASS(type) struct Object { type value = 3; }



VALUE_CLASS(int);


int main(int input)
{
	Object x;
	
	return x.value;
}

};

}
namespace preprocessor_test{
namespace nested_endif
{


#if 0
#define X 125125

#if 0
#define Y asags
#endif

MUST NOT BE COMPILED!!!%$$

#endif

int main(int input)
{
	return 12;
}

};

}
namespace preprocessor_test{
namespace nested_macro
{

#define MUL(a, b) a * b
#define ADD(a, b) a + b

int main(int input)
{
	return MUL((ADD(2, 3)), 4);
}

};

}
namespace preprocessor_test{
namespace preprocessor_constant
{

#define MY_CONSTANT 125

int main(int input)
{
	return MY_CONSTANT;
}

};

}
namespace preprocessor_test{
namespace preprocessor_if1
{


#if 0
int x = 12;
#else
int x = 190;
#endif

int main(int input)
{
	return x;
}

};

}
namespace preprocessor_test{
namespace preprocessor_if2
{

#define ENABLE_FIRST 1


#if ENABLE_FIRST
#if true
int x = 92;
#else
int x = 12;
#endif
#else
int x = 190;
#endif

int main(int input)
{
	return x;
}

};

}
namespace preprocessor_test{
namespace preprocessor_if4
{

#define ENABLE_FIRST 0


int main(int input)
{
    #if ENABLE_FIRST
	return x;
	#else
	return 10;
	#endif
}

};

}
namespace preprocessor_test{
namespace preprocessor_multiline
{


#define XXX(name) \
int name() { \
  return 12; \
}

XXX(test)


int main(int input)
{
    return test();
}
};

}
namespace preprocessor_test{
namespace preprocessor_sum
{

#define SUM(a, b) a + b

int main(int input)
{
	return SUM(input, 12);
}

};

}
namespace preprocessor_test{
namespace template_macro
{

#define func(x) template <int C> void x(int& value) { value = C; }

func(setToValue);


int main(int input)
{
	setToValue<5>(input);
	
	return input;
}

};

}
namespace ra_pass_test{
namespace math_noop
{

int main(int input)
{
	return input * 1;
}

};

}
namespace ra_pass_test{
namespace math_noop_double
{

double main(double input)
{
	input *= 10.0;
	

	return input + 0.0;
}

};

}
namespace ra_pass_test{
namespace math_noop_float
{

float main(float input)
{
	input *= 10.0f;

	return input + 0.0f;
}

};

}
namespace ra_pass_test{
namespace skip_float
{



int main(int input)
{

	float x = 12.0f;
	x = 19.0f;
	
	return (int)x;
}

};

}
namespace ra_pass_test{
namespace skip_memory_write
{

span<float, 2> d = { 12.0f };

int main(int input)
{
	d[1] = 19.0f;
	d[1] = 12.0f;
	
	return 12;
}

};

}
namespace ra_pass_test{
namespace skip_mov
{

int x = 12;

int main(int input)
{
	
	x = 19;
	x = 20;
	
	return x;
}


};

}
namespace span_test{
namespace add_float4_float5
{



float main(float input)
{
    span<float, 5> d5 = { 1.0f };
    span<float, 4> d4 = { 0.5f };
    
    
	return d4[2] + d5[1];
}

};

}
namespace span_test{
namespace chainprocessingofstructs
{

using T = float;

struct Multiplier
{
    T op(T in)
    {
        return in * gain;
    }
    
    T gain = 1.0f;
};

span<Multiplier, 3> mchain;

float main(float input)
{
    mchain[0].gain = 0.5f;
    mchain[1].gain = 0.25f;
    mchain[2].gain = 2.0f;

	for(auto& g: mchain)
  {
	    input = g.op(input);
  }

	  
	return input;
}

};

}
namespace span_test{
namespace float4_as_arg
{

float4 data = { 1.0f, 2.0f, 3.0f, 4.0f };

void clearFloat4()
{
	data = 2.0f;
}

int main(int input)
{
	clearFloat4();
	
	return (int)data[3];
}

};

}
namespace span_test{
namespace get
{

span<int, 2> data = { 1, 3};

int main(int input)
{
    return data[1];
}

};

}
namespace span_test{
namespace local_definition
{

span<float, 8> d = { 4.0f, 5.0f, 6.0f, 7.0f, 
                     8.0f, 9.0f, 10.0f, 11.0f };

float main(float input)
{
    float4 v = { 1.0f, 0.0f, 1.0f, 0.0f };
    
    float sum = 0.0f;
    
    
    for(auto& s: d.toSimd())
    {
        s *= v;
    }
    
    
    for(auto& s: d)
        sum += s;
        
    
    return sum;
}

};

}
namespace span_test{
namespace local_definition_scalar
{

span<float, 8> d = { 4.0f, 5.0f, 6.0f, 7.0f, 
                     8.0f, 9.0f, 10.0f, 11.0f };

float main(float input)
{
    float4 v = { 0.5f };
    
    float sum = 0.0f;
    
    for(auto& s: d.toSimd())
    {
        s *= v;
    }
    
    for(auto& s: d)
        sum += s;
    
    return sum;
}

};

}
namespace span_test{
namespace local_definition_without_alias
{


float main(float input)
{
    span<float, 2> data = { 2.0f, 3.0f };
    
    return data[0] + data[1];
}

};

}
namespace span_test{
namespace local_spanandwrap
{

int main(int input)
{
	span<int, 3> d = { 1, 2, 3 };
	span<int, 3>::wrapped i = 4;
	
	return d[i];
	
	
}

};

}
namespace span_test{
namespace local_span_anonymous_scope
{

struct Object
{
    int z = 5;
    double d = 1.0;
};

float main(float input)
{
	Object d1, d2;
	
	return (float)d1.d + (float)d2.d;
}

};

}
namespace span_test{
namespace local_span_as_function_parameter
{

using TwoInts = span<int, 2>;

void clearSpan(TwoInts& data)
{
    data[0] = 3;
    data[1] = 3;
}

int main(int input)
{
	TwoInts c = { 9, 1 };
	
	clearSpan(c);
	
	return c[0] + c[1];
}

};

}
namespace span_test{
namespace loop_break_continue
{

span<int, 5> d = { 1, 2, 3, 4, 5 };



int main(int input)
{
    int sum = 0;
    
	for(auto& s: d)
    {
        if(s == 3)
            continue;
        
        if(s == 5)
            break;
            
        sum += s;
    }
    
    return sum;
}

};

}
namespace span_test{
namespace loop_break_continue_nested1
{

span<span<float, 3>, 5> d = { {1.0f, 2.0f, 3.0f}};

int main(int input)
{
    float sum = 0.0f;
    
	for(auto& i: d)
    {
        for(auto& s: i)
        {
            if(s == 3.0f)
                break;

            sum += s;
        }
    }
    
    return (int)sum;
}

};

}
namespace span_test{
namespace loop_break_continue_nested2
{

span<span<float, 3>, 5> d = { {1.0f, 2.0f, 3.0f}};

int main(int input)
{
    float sum = 0.0f;
    
    int counter = 0;
    
	for(auto& i: d)
    {
        if(counter++ == 4)
            break;
        
        for(auto& s: i)
        {
            if(s == 2.0f)
                continue;
            
            sum += s;
        }
    }
    
    return (int)sum;
}

};

}
namespace span_test{
namespace loop_nested
{

span<span<float, 3>, 5> d = { {1.0f, 2.0f, 3.0f}};

int main(int input)
{
    float sum = 0.0f;
    
	  for(auto& i: d)
    {
        for(auto& s: i)
        {
            sum += s;
        }
    }
    
    return (int)sum;
}

};

}
namespace span_test{
namespace nested_span_copy
{


span<span<int, 4>, 2> data = { {3}, {2} };

int main(int input)
{
	auto d = data[0];
	
	d[1] = 9;
	data[0][1] = 18;

	return d[1] + data[0][1];
}
};

}
namespace span_test{
namespace sequential_write
{

int z = 8;
double v = 9.0;
span<float, 3> d = { 1.0f, 2.0f, 3.0f};

int main(int input)
{
    {
	    auto& s = d[0];
	    s += 8.0f;
    }
    
    
    {
        auto& s = d[0];
        input += (int)s;
    }
    
    
	
	return input;
}

};

}
namespace span_test{
namespace set
{

span<int, 5> data;

void setValue()
{
    data[2] = 4;
}


int main(float input)
{
    setValue();
	
    return data[2];
}

};

}
namespace span_test{
namespace simd_add
{

//span<float, 8> d = { 2.0f };

span<float, 8> d = { 2.0f };
float4 x = { 1.0f, 2.0f, 3.0f, 4.0f };

float main(float input)
{
    float sum = 0.0f;
    
    for(auto& v: d.toSimd())
    {
        v += x;
    }

    for(auto& v: d)
        sum += v;

    return sum;
}

};

}
namespace span_test{
namespace simd_add_scalar
{

span<float, 8> d = { 1.0f, 2.0f, 3.0f, 4.0f, 1.0f, 2.0f, 3.0f, 4.0f };

float4 x = { 1.0f, 2.0f, 3.0f, 4.0f };

float main(float input)
{
    for(auto& v: d.toSimd())
    {
        v += 4.0f;
    }
    
    float sum = 0.0f;
    
    for(auto& s: d)
        sum += s;
    
    return sum;
}

};

}
namespace span_test{
namespace simd_dyn_index
{


using T = double;

span<T, 4> d = { (T)1, (T)2, (T)3, (T)4 };

int main(int input)
{
    
    span<T, 4>::wrapped i = input;
	
    
	return d[i];
}

};

}
namespace span_test{
namespace simd_mul
{

span<float, 8> d = { 2.0f };

float4 x = { 0.5f };

float main(float input)
{
    float sum = 0.0f;
    
    for(auto& v: d.toSimd())
    {
        v *= x;
    }
    
    for(auto& v: d)
        sum += v;
    
    return sum;
}

};

}
namespace span_test{
namespace simd_mul_scalar
{

span<float, 8> d = { 2.0f };

float main(float input)
{
    for(auto& v: d.toSimd())
    {
        v *= 0.5f;
    }
    
    float sum = 0.0f;
    
    for(auto& s: d)
        sum += s;
    
    return sum;
}

};

}
namespace span_test{
namespace simd_set
{

span<float, 8> d;

float4 x = { 1.0f, 2.0f, 3.0f, 4.0f };

float main(float input)
{
    float sum = 0.0f;
    
    for(auto& v: d.toSimd())
    {
        v = x;
    }
    
    for(auto& v: d)
        sum += v;
    
    return sum;
}

};

}
namespace span_test{
namespace simd_set_scalar
{

span<float, 12> d = { 2.0f };



float main(float input)
{
    for(auto& v: d.toSimd())
    {
        float x = 4.0f;
        v = x;
    }
    
    float sum = 0.0f;
    
    for(auto& s: d)
        sum += s;
    
    return sum;
}
};

}
namespace span_test{
namespace simd_set_self_scalar
{

span<float, 8> d = { 4.0f, 5.0f, 6.0f, 7.0f, 
                     8.0f, 9.0f, 10.0f, 11.0f };

float main(float input)
{
    float sum = 0.0f;
    
    for(auto& s: d.toSimd())
    {
        s = s[1];
    }
    
    
    for(auto& s: d)
        sum += s;
        
        
    
        
    return sum;
}

};

}
namespace span_test{
namespace simd_sum_self_scalar
{

span<float, 8> d = { 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f };

float main(float input)
{
    float sum = 0.0f;
    
    for(auto& s: d.toSimd())
    {
        // this should add 5.0f to the first half
        // and 9.0f to the second half
        s += s[1];
    }
    
    for(auto& s: d)
        sum += s;
    
    return sum;
}

};

}
namespace span_test{
namespace simd_sum_self_scalar_no_opt
{

span<float, 8> d = { 4.0f, 5.0f, 6.0f, 7.0f, 
                     8.0f, 9.0f, 10.0f, 11.0f };

float main(float input)
{
    float sum = 0.0f;
    
    float x = 0.0f;
    
    for(auto& s: d.toSimd())
    {
        // this should add 9.0f to the first half
        // and 17.0f to the second half
        s += s[1] + s[0];
    }
    
    for(auto& s: d)
        sum += s;
    
    // float expected = 60.0f + 4.0f * 9.0f + 4.0f * 17.0f;
        
    return sum;
}

};

}
namespace span_test{
namespace slice_test
{

span<int, 8> d = { 1, 2, 3, 4, 5, 6, 7, 8 };

int main(int input)
{
	auto refX = slice(d, 7, 2);
	
	// change the actual data
	refX[0] = 90;

	// should return 90...
	return d[2];
}

};

}
namespace span_test{
namespace small_object_register
{

int main(int input)
{
	span<float, 1> d = { 3.5f };
	return (int)d[0];
}

};

}
namespace span_test{
namespace span_1element
{

int main(int input)
{
	span<int, 1> obj = { 3 };
	
	return obj[0];
}

};

}
namespace span_test{
namespace span_double_index
{

span<span<int, 2>, 3> data = {{1, 2},{3, 4},{5, 6}};

span<span<int, 2>, 3>::unsafe i;


int main(int input)
{
    i = 1;
	return data[i][1];
}

};

}
namespace span_test{
namespace span_make_index
{

using SpanType = span<int, 5>;
using WrapType = SpanType::wrapped;

SpanType d = { 1, 2, 3, 4, 5 };

int main(int input)
{
	return d[d.index<WrapType>(input)];
}

};

}
namespace span_test{
namespace span_size
{

span<int, 5> d;

int main(int input)
{
    return d.size();
}};

}
namespace span_test{
namespace span_with_simplecomplex_type
{



struct X
{
    int v = 12;
};

using MySpan = span<X, 2>;

MySpan x;

int main(int input)
{
    return x[0].v;
}

};

}
namespace span_test{
namespace to_simd
{

span<float, 8> d = { 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f };

float main(float input)
{
    float sum = 0.0f;
    
    for(auto& s: d.toSimd())
    {
        sum += s[0];
    }
    
    return sum;
}

};

}
namespace span_test{
namespace two_local_spans
{


float main(float input)
{
	span<float, 2> d1 = { 1.0f, 2.0f };
	span<float, 5> d2 = { 6.0f};
	
	return d1[0] + d2[4];
}

};

}
namespace span_test{
namespace unsafe_1
{

span<float, 9>::unsafe i = { 12 };

int main(int input)
{
	return i;
}

};

}
namespace span_test{
namespace unsafe_3
{

span<float, 9>::unsafe i;

int main(int input)
{
    i = 91;
	return ++i;
}

};

}
namespace span_test{
namespace unsafe_4
{

span<float, 9>::unsafe i;

int main(int input)
{
    i = 91;
	return i.moved(9);
}

};

}
namespace span_test{
namespace wrapped_initialiser
{

span<int, 5>::wrapped s = { 7 };

int main(int input)
{
	return s;
}

};

}
namespace span_test{
namespace wrap_dec
{

span<int, 8>::wrapped i;

int main(int input)
{
	
	
	return --i;
}

};

}
namespace span_test{
namespace wrap_index_simple
{

using SpanType = span<float, 5>;


SpanType d = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f };

SpanType::wrapped i;
span<float, 5>::wrapped j;

int main(int input)
{
	i = 4;
	++i;
	j = 8;

	return (int)d[i] + (int)d[j];
}

};

}
namespace span_test{
namespace wrap_local_def
{



int main(int input)
{
	span<int, 8>::wrapped i;
	
	return --i;
}

};

}
namespace span_test{
namespace wrap_moved
{

span<int, 8>::wrapped i = { 5 };

int main(int input)
{
    return i.moved(5);
}

};

}
namespace span_test{
namespace wrap_moved1
{

span<int, 8>::wrapped i = { 5 };

int main(int input)
{
    return i.moved(i);
}

};

}
namespace span_test{
namespace wrap_moved2
{

span<int, 8>::wrapped i = { 5 };

int main(int input)
{
    return i.moved(++i);
}

};

}
namespace span_test{
namespace wrap_moved3
{

span<int, 4> d = { 1, 2, 3, 4 };

span<int, 4>::wrapped i;

int main(int input)
{
    return d[i.moved(-1)];
}

};

}
namespace span_test{
namespace wrap_sum
{


span<int, 8>::wrapped i = { 5 };

int main(int input)
{
	return input + i;
}

};

}
namespace struct_test{
namespace basic_copy
{

struct X
{
    
    float d = 12.0f;
    int x = 9;
    double f = 16.0;
};

X x;


void change(X obj)
{
    obj.x = 5;
}

int main(int input)
{
	change(x);
	
	return x.x;
}

};

}
namespace struct_test{
namespace complex_member_call
{


struct X
{
    int x = 5;
    
    int getInner()
    {
        return x;
    }
    
    int getX(int b)
    {
        return b > 5 ? getInner() : 9; 
    }
    
    
};

int main(int input)
{
    X obj;
    
    return obj.getX(input);
}
};

}
namespace struct_test{
namespace double_member_2
{



struct X 
{ 
    struct Y 
    { 
        int value = 19; 
    }; 
    
    Y y; 
}; 

X x; 

int main(int input) 
{ 
    x.y.value = input + 5; 
    int v = x.y.value; 
    return v; 
}};

}
namespace struct_test{
namespace double_struct_member
{

struct X
{
    struct Y
    {
        int z = 12;
    };
    
    Y y;
};

X x;

int main(int input)
{
	x.y.z = 16;
	x.y.z = 15;
	
	return x.y.z;
}

};

}
namespace struct_test{
namespace inner_struct
{

struct X
{
    struct Y
    {
        int z = 5;
    };
};

X::Y obj;

int main(int input)
{
	return obj.z;
}

};

}
namespace struct_test{
namespace local_struct_definition
{


using T = float;

struct X
{
    T v1 = 0.0f;
    T v2 = 0.f;
    T v3 = 0.f;
};



T main(T input)
{
    X v = { 2.0f, 3.0f, 4.0f };
    
    return v.v1 + v.v2;
}

};

}
namespace struct_test{
namespace local_struct_mask_global
{


using T = float;

struct X
{
    T v1 = 2.0f;
    T v2 = 3.f;
    T v3 = 0.f;
};

X v = { 8.0f, 0.0f, 1.0f };

T main(T input)
{
    X v;
    
    return v.v1 + v.v2;
}

};

}
namespace struct_test{
namespace local_struct_ref
{

struct X
{
    int v = 12;
    float d = 12.0f;
};

span<X, 12> data;

int main(int input)
{
	auto x = data[1];
	
	x.d = 15.0f;	
	return x.v + (int)x.d;
}

};

}
namespace struct_test{
namespace local_struct_with_default
{


using T = float;

struct X
{
    T v1 = 2.0f;
    T v2 = 3.f;
    T v3 = 0.f;
};



T main(T input)
{
    X v;
    
    return v.v1 + v.v2;
}

};

}
namespace struct_test{
namespace member_set_function
{

struct X
{
  int v = 1;

  void setV(int value)
  {
    v = value;
  }
};

X x;

int main(int input)
{
	x.setV(90);
  return x.v;
}

};

}
namespace struct_test{
namespace method_arg_override
{


struct X
{
    int x = 5;
    
    int getX(int x)
    {
        return x;
    }
};

X obj;

int main(int input)
{
    return obj.getX(input);
}
};

}
namespace struct_test{
namespace nested_init_with_constructor
{


struct MyFirstStruct
{
	struct InnerStruct
	{
		int i1 = 0;
		int i2 = 1;
	};

	MyFirstStruct(int initValue)
	{
		is1.i1 = initValue * 6;
		is2.i2 = initValue * 4;
	}
	
	int get() const
	{
		return is1.i1 + is1.i2 + is2.i2 + is2.i1;
	}

	InnerStruct is1;
	InnerStruct is2;	

}; 


int main(int input)
{
	MyFirstStruct localObject = { 3 };	
	
	return localObject.get();
}

};

}
namespace struct_test{
namespace nested_member_call
{


struct X
{
    int x = 12;
    
    int first()
    {
        return x;
    }
    
    int second()
    {
        return first();
    }
};

X obj;

int main(int input)
{
	return obj.second();
}

};

}
namespace struct_test{
namespace simd_on_local_member_span
{


using T = float;

struct X
{
    span<float, 8> s;
};



T main(T input)
{
    X x2 = { { 7.0f } };
    
    
    for(auto& v: x2.s.toSimd())
    {
        v = 9.0f;
    }
    
    return x2.s[0];
}

};

}
namespace struct_test{
namespace simd_on_member_span
{


using T = float;

struct X
{
    span<float, 8> s;
};


X x2 = { { 7.0f } };

T main(T input)
{
    for(auto& v: x2.s.toSimd())
    {
        v = 9.0f;
    }
    
    return x2.s[0];
}

};

}
namespace struct_test{
namespace span_as_ref_param
{

using TwoInts = span<int, 2>;

void clearSpan(TwoInts& data)
{
    data[0] = 3;
    data[1] = 3;
}

int main(int input)
{
	TwoInts c = { 9, 1 };
	
	clearSpan(c);
	
	return c[0] + c[1];
}

};

}
namespace struct_test{
namespace span_struct_member_register
{

struct X
{
    int z = 12;
};

span<X, 4> d;

int main(int input)
{
	d[3].z = 13;
	
	return d[3].z;
}

};

}
namespace struct_test{
namespace struct_definition
{

struct X
{
    float first = 1.0f;
    float second = 0.0f;
};

X v = { 2.0f, 3.0f };

float main(float input)
{
    
    
    
    return v.first + v.second;
}

};

}
namespace struct_test{
namespace struct_member_call
{


struct X
{
    int v = 120;
    
    int getX()
    {

        return v;
    }
};

X x;

int main(int input)
{
    //return x.v;

	  return x.getX();
}

};

}
namespace struct_test{
namespace struct_ref2function
{

struct X
{
    float z = 129.0f;
};

void change(X& obj)
{
    obj.z = 90.0f;
}

X x;

int main(int input)
{
	change(x);
	
	return (int)x.z;
}

};

}
namespace struct_test{
namespace sum_reference_func
{

struct X
{
    int x1 = 2;
    int x2 = 54;
};

span<float, 4> data = { 2.0f };

int sumX(X& x)
{
    return x.x1 + x.x2;
}


int main(int input)
{
	X x;
	
  return sumX(x);
}



};

}
namespace struct_test{
namespace two_local_structs
{


using T = float;

struct X
{
    T v1 = 0.0f;
    T v2 = 0.f;
    T v3 = 0.f;
};



T main(T input)
{
    X x1 = { 2.0f, 3.0f, 4.0f};
    X x2 = { 1.0f, 0.0f, 0.0f };
    
    return x1.v2 + x2.v1;
}

};

}
namespace template_test{
namespace complex_template1
{

struct Outer
{
    static const int C1 = 5;
    
    template <int C2> struct Inner
    {
        int get()
        {
            return C1 + C2;
        }
    };
};

Outer::Inner<3> d;

int main(int input)
{
	return d.get();
}

};

}
namespace template_test{
namespace complex_template2
{


template <int C1> struct Outer
{
    template <int C2> struct Inner
    {
        int get()
        {
            return C1 + C2;
        }
    };
};

Outer<5>::Inner<3> d;


int main(int input)
{
  return d.get();
}

};

}
namespace template_test{
namespace complex_template4
{


span<int, 13> d = { 9 };

template <typename T> T getFirst(span<T, 13>& data)
{
    return data[0];
}


int main(int input)
{
  // check with span: maybe add ComplexType::getTemplateArguments...
    return getFirst(d);
}};

}
namespace template_test{
namespace external_template_function
{

using SpanType = span<int, 4>;

SpanType d = { 1, 2, 3, 4 };

int main(int input)
{
	auto i = d.index<SpanType::wrapped>(6);
	
	return d[i];
	
}

};

}
namespace template_test{
namespace simple_template
{

template <typename T> struct X
{
    T x = (T)2;
};

X<int> c;

int main(int input)
{
	return c.x;
}

};

}
namespace template_test{
namespace simple_template10
{

template <typename T> struct X
{
    T obj;
};

X<float> f = { 8.0f };

X<span<int, 3>> s = { {1, 2, 3} };

int main(int input)
{
	return (int)f.obj + s.obj[0] + s.obj[1];
}

};

}
namespace template_test{
namespace simple_template13
{

template <int C> int multiply(int x)
{
    return x * C;
}

int main(int input)
{
	return multiply<5>(6);
}

};

}
namespace template_test{
namespace simple_template14
{

template <int C> int multiply(int x)
{
    return x * C;
}

int main(int input)
{
	return multiply<5>(6) + multiply<2>(6);
}

};

}
namespace template_test{
namespace simple_template15
{

template <typename T> T multiply(T a, T b)
{
    return a * b;
}

int main(int input)
{
	return multiply<int>(input, 12);
}

};

}
namespace template_test{
namespace simple_template16
{

template <typename T> T multiply(T a, T b)
{
    return a * b;
}

int main(int input)
{
	return multiply(input, 12);
}

};

}
namespace template_test{
namespace simple_template17
{

template <int C, typename T> T multiplyC(T a, T b)
{
    return C + a * b;
}

int main(int input)
{
	return multiplyC<5>(input, 12);
}

};

}
namespace template_test{
namespace simple_template18
{

// Checks whether the const & ref modifiers will be 
// correctly detected during template argumenty type deduction..


struct X
{
    int x = 5;
};

template <typename T> void change(T& obj)
{
    obj.x = 8;
}

X x;

int main(int input)
{
	change(x);
	return x.x;
	
}

};

}
namespace template_test{
namespace simple_template19
{


template <int C1> struct X
{
    template <int C2> int multiply()
    {
        return C1 * C2;
    }
};

X<5> x;

int main(int input)
{
    return x.multiply<3>();
}

};

}
namespace template_test{
namespace simple_template2
{

template <typename T> struct X
{
    T x = (T)2;
};

X<int> c;
X<float> f;

int main(int input)
{
	return c.x + (int)f.x;
}

};

}
namespace template_test{
namespace simple_template20
{


template <int C1> struct X
{
    template <int C2> int multiply()
    {
        return C1 * C2;
    }
};

X<2> x2;
X<3> x3;

int main(int input)
{
    int ten = x2.multiply<2>() + x2.multiply<3>();
    int v23 = x2.multiply<4>() + x3.multiply<5>();
    
    return ten + v23;
    
}

};

}
namespace template_test{
namespace simple_template21
{


template <int C1> struct X
{
    template <int C2> int multiply()
    {
        return C1 * C2;
    }
};

X<2> x2;

int main(int input)
{
    return x2.multiply<3>();
}

};

}
namespace template_test{
namespace simple_template22
{


template <typename T, int NumElements> struct X
{
    T& get()
    {
        return data[1];
    }
    
    span<T, NumElements> data = { 1, 2 };
};

X<int, 2> data;

int main(int input)
{
	auto v = data.get();
	
	return v;
}

};

}
namespace template_test{
namespace simple_template23
{


struct X
{
    double a = 1.0;
    double b = 2.0;
    
    
    void setA(double v)
    {
        a = v;
    }
    
    void setB(double v)
    {
        b = v;
    }
    
    template <int T> void set(double v)
    {
        if(T == 0)
            setA(v);
        if(T == 1)
            setB(v);
        
        return;
    }
    
};

X obj;

int main(int input)
{
	obj.set<0>(20.0);
	
	return (int)obj.a;
}

};

}
namespace template_test{
namespace simple_template24
{

template <int C> struct X
{
    int get()
    {
        return C;
    };
};

int main(int input)
{
	X<5> obj;
	
	return obj.get();
}

};

}
namespace template_test{
namespace simple_template25
{

template <typename T> struct Dual
{
    T sum()
    {
        return data[0] + data[1];
    }
    
    span<T, 2> data;
};




double main(double input)
{
	Dual<double> d = { {1.0, 3.0} };
	
	return d.sum();
}

};

}
namespace template_test{
namespace simple_template26
{

template <int NumElements> struct X
{
    span<float, NumElements> data = { 4.0f };
};

float main(float input)
{
  
	X<4> obj1 = { { 0.5f }};
  X<4> obj2;
	
	
	
	return obj1.data[0] + obj2.data[2];
}

};

}
namespace template_test{
namespace simple_template27
{

template <int NumElements> struct X
{
    span<float, NumElements> data = { 4.0f };
};

float main(float input)
{
  X<4> obj2;
  X<5> obj1 = { { 0.5f }};
  
	
	
	
	return obj1.data[0] + obj2.data[2];
}

};

}
namespace template_test{
namespace simple_template28
{


template <int C> int multiply(int input)
{
    return input * C;
}

int main(int input)
{
	return multiply<5>(input) + multiply<5>(input);
}

};

}
namespace template_test{
namespace simple_template29
{

template <typename T> struct X
{
    template <int Index> T get()
    {
        return data[Index];
    }
    
    span<T, 3> data;
};

X<float> obj = { {1.0f, 2.0f, 7.0f} };

int main(int input)
{
	return obj.get<0>() + obj.get<2>();
}

};

}
namespace template_test{
namespace simple_template3
{

template <typename T> struct X
{
    T getSquare()
    {
        return x * x;
    }
    
    T x = (T)2;
};

X<int> c;

int main(int input)
{
	return c.getSquare();
}

};

}
namespace template_test{
namespace simple_template4
{

template <typename T> struct X
{
    T multiply(T v)
    {
        return x * v;
    }
    
    T x = (T)2;
};

X<int> c;

int main(int input)
{
	return c.multiply(input);
}

};

}
namespace template_test{
namespace simple_template5
{

template <typename T> struct X
{
    T multiply(T v)
    {
        return x * v;
    }
    
    T x = (T)2;
};

X<int> c;
X<int> c2;

int main(int input)
{
    c2.x = 1;
    
	return c.multiply(input) + c2.multiply(input);
}

};

}
namespace template_test{
namespace simple_template6
{

template <int MaxSize=90> struct X
{
    int getX()
    {
      return x * MaxSize;
    }

    int x = 2;
};

X<5> c;
X<8> c2;

int main(int input)
{
    c2.x = 1;
    
	  return c.getX() + c2.getX();
}

};

}
namespace template_test{
namespace simple_template7
{

template <int First, int Second=9> struct X
{
    int get()
    {
        return First + Second;
    }
};

X<12> c;

int main(int input)
{
    return c.get();
}

};

}
namespace template_test{
namespace simple_template8
{


template <typename T> struct ArraySize
{
    T t;
    
    int getSize()
    {
        return t.size();
    }
};

ArraySize<span<int, 5>> d = { {9} };

int main(int input)
{
  return d.getSize() + d.t[0];
}

};

}
namespace template_test{
namespace simple_template9
{


template <typename S> struct X
{
    span<S, 5> data = { 4 };
};

X<int> d;


int main(int input)
{
	return (int)d.data[0];
    
}

};

}
namespace template_test{
namespace span_in_template
{


template <int NumElements, typename T> struct X
{
    span<T, NumElements> x = { (T)2 };
};

X<4, int> c;


int main(int input)
{
	return c.x[0];
}

};

}
namespace template_test{
namespace static_function_template
{


struct X
{
	template <int T> static int getValue()
	{
		return T;
	}
};

int main(int input)
{
	return X::getValue<5>();
}

};

}
namespace template_test{
namespace template_overload1
{


template <typename Last> int add()
{
	return Last::value;
}


template <typename T1, typename T2> int add()
{
  return T1::value + T2::value;
}

struct C1
{
  static const int value = 5;
};

struct C2
{
  static const int value = 8;
};


int main(int input)
{
 	return add<C1, C2>() + add<C2>();
}

};

}
namespace template_test{
namespace template_static_function
{


struct C
{
  static int get() { return 9; }
};

template <typename T> int callSomething()
{
  return T::get();
}




int main(int input)
{
	return callSomething<C>();
}

};

}
namespace template_test{
namespace template_static_member
{

template <typename T> int getSomething()
{
	return T::value;
}

struct C
{
	static const int value = 9;
};

int main(int input)
{
	return getSomething<C>();
}

};

}
namespace template_test{
namespace variadic_template1
{

#if 0
template <typename First, typename... Ts> int sum()
{
	return First::value + sum<Ts...>();
}

template <typename Last> int sum()
{
	return Last::value;
}

struct A
{
	static const int value = 5;
};

struct B
{
	static const int value = 4;
}; 
#endif

int main(int input)
{
#if 0
  return sum<A, A, B>();
#else
	return 14;
#endif
}

};

}
namespace variadic_test{
namespace nested_get
{



struct X
{
    DECLARE_NODE(X);

    static const int NumChannels = 2;

    template <int P> void setParameter(double v)
    {
      
    }

    int v = 5;
    double x = 2.0;
    span<float, 4> data = { 2.0f, 0.5f, 1.0f, 9.2f };
};



container::chain<parameter::empty, wrap::fix<2, X>, container::chain<parameter::empty, X, X>> c;

int main(int input)
{
	return (int)c.get<1>().get<1>().data[3];
}

};

}
namespace variadic_test{
namespace nested_reset
{

struct X
{
    DECLARE_NODE(X);

    template <int P> void setParameter(double v)
    {
        
    }

    int v = 90;
    
    void reset()
    {
        v = 1;
    }
};

container::chain<parameter::empty, container::chain<parameter::empty, wrap::fix<1, X>, X>, X, X> c;

int main(int input)
{
	c.reset();
	
  return c.get<1>().v + c.get<2>().v;
}

};

}
namespace variadic_test{
namespace parse_getter
{

struct X
{
    DECLARE_NODE(X);
    static const int NumChannels = 1;

    template <int P> void setParameter(double v)
    {
      
    }
    
    int v = 8;
};

container::chain<parameter::empty, X, X> c;

int main(int input)
{
	c.get<1>().v = input * 2;
	
	return c.get<1>().v;
}

};

}
namespace variadic_test{
namespace prepare_call
{

struct X
{
    DECLARE_NODE(X);

    template <int P> void setParameter(double v)
    {
      
    }

    int x = 0;
    
    void prepare(PrepareSpecs ps)
    {
        x = ps.blockSize;
        ps.blockSize = 100;
    }
};

struct Y
{
    DECLARE_NODE(Y);

    template <int P> void setParameter(double v)
    {
      
    }

    int value = 0;
    
    void prepare(PrepareSpecs ps)
    {
        value = ps.blockSize * 2;
    }
};

container::chain<parameter::empty, wrap::fix<2, X>, Y> c;


int main(int input)
{
	PrepareSpecs ps = { 44100.0, 512, 2 };
	
	c.prepare(ps);
	
    return c.get<0>().x;

//	return ps.blockSize;
}

};

}
namespace variadic_test{
namespace prepare_definition
{

PrepareSpecs ps = { 44100.0, 512, 2 };

int main(int input)
{
	return ps.blockSize;
}

};

}
namespace variadic_test{
namespace prepare_specs_pass_by_value
{


void clearPrepareSpecs(PrepareSpecs ps)
{
    ps.blockSize = 9;
}

int main(int input)
{
	PrepareSpecs ps = { 44100.0, 512, 2 };
	
	clearPrepareSpecs(ps);
	
	return ps.blockSize;
}

};

}
namespace variadic_test{
namespace process_single_test
{

struct X
{
    DECLARE_NODE(X);

    template <int P> void setParameter(double v)
    {
      
    }

    void processFrame(span<float, 1>& data)
    {
        data[0] *= 0.5f;
    }
};

struct Y
{
    DECLARE_NODE(Y);

    template <int P> void setParameter(double v)
    {
      
    }

    void processFrame(span<float, 1>& data)
    {
        data[0] += 2.0f;
    }
};

container::chain<parameter::empty, wrap::fix<1, X>, Y, X> c;

span<float, 1> d = { 1.0f };

float main(float input)
{
	 c.processFrame(d);
    
    return d[0];
}};

}
namespace variadic_test{
namespace process_test
{



int main(int input)
{
    return 112;
}

};

}
namespace variadic_test{
namespace reset_method
{

struct X
{
    DECLARE_NODE(X);

    template <int P> void setParameter(double v)
    {
      
    }

    static const int NumChannels = 2;

    void reset()
    {
        v = 1;
    }
    
    int v = 8;
};

container::chain<parameter::empty, X, X, X> c;

int main(int input)
{
	c.reset();
	
	return c.get<1>().v + c.get<0>().v;
}

};

}
namespace variadic_test{
namespace simple_reset
{

namespace funky
{

    struct X
    {
        DECLARE_NODE(X);

        template <int P> void setParameter(double v)
        {
          
        }

        void reset()
        {
            
        }
    };
}

container::chain<parameter::empty, wrap::fix<1, funky::X>, funky::X> c;

int main(int input)
{
	c.reset();
	return 12;
}

};

}
namespace variadic_test{
namespace simple_variadic_test
{

struct X
{
    DECLARE_NODE(X);

    template <int P> void setParameter(double v)
    {
      
    }

    int v = 0;
    
    void process(int input)
    {
        v = input;
    }
};

container::chain<parameter::empty, wrap::fix<1, X>, X> c;

int main(int input)
{
    return 12;
}};

}
namespace variadic_test{
namespace split_test
{

struct Test
{
	DECLARE_NODE(Test);

	template <int P> void setParameter(double v)
	{
		
	}

	void reset()
	{
		
	}
};

container::split<parameter::empty, wrap::fix<1, Test>, Test> s;

int main(int input)
{
	s.reset();
	

	return 25;
}

};

}
namespace variadic_test{
namespace variadic_parser
{

struct X
{
    DECLARE_NODE(X);

    template <int P> void setParameter(double d)
    {
      
    }

    static const int NumChannels = 1;

    int v = 0;
    
    
};

container::chain<parameter::empty, wrap::fix<1, X>, container::chain<parameter::empty, X, X>> c;

int main(int input)
{
    return 12;
}

};

}
namespace variadic_test{
namespace wrap_event
{

struct X
{
	DECLARE_NODE(X);

	template <int P> void setParameter(double v) {}

    int z = 12;
};

wrap::event<X> m;

int main(int input)
{
	return m.obj.z;
}

};

}
namespace variadic_test{
namespace wrap_event_funky
{

struct X
{
  DECLARE_NODE(X);

  template <int P> void setParameter(double v) {}
  
    void reset()
    {
        x = 90;
    }
    
    int x = 12;
};

wrap::event<X> m;

int main(int input)
{
  m.reset();
	return m.obj.x;
}

};

}
namespace visibility_test{
namespace private_member_by_method
{


class X
{
	int x = 9;

public:
	
	int getX() const
	{
		return x;
	}
};


X obj;

int main(int input)
{
	return obj.getX();
}

};

}
namespace visibility_test{
namespace public_enum
{


class X
{
public:

	enum MyEnum
	{
		First = 10
	};
};


int main(int input)
{
	return X::MyEnum::First;
}

};

}
namespace vop_test{
namespace vop2
{

span<float, 5> data = { 0.5f};

int main(int input)
{
	data *= -1.0f;
	return 12;
}

};

}
namespace vop_test{
namespace vop_1
{

span<float, 128> data = {0.0f};

dyn<float> d;

int main(int input)
{
	d.referTo(data);
	
	d *= 2.0f;

	return 12;
	
}

};

}
namespace vop_test{
namespace vop_2
{

span<float, 129> data = {20.0f};

dyn<float> d;

float main(float input)
{
	d.referTo(data);
	
	d *= 2.0f;

	return d[128];
	
}

};

}
namespace vop_test{
namespace vop_3
{

span<float, 128> data = {20.0f};

dyn<float> d;

span<float, 128> data2 = {3.0f};

dyn<float> d2;

float main(float input)
{
	d.referTo(data, 64);
	d2.referTo(data2, 64);
	
	d *= d2;

	return d[32];
	
}

};

}
namespace vop_test{
namespace vop_4
{

span<float, 128> data = {20.0f};

dyn<float> d;
dyn<float> d2;

span<float, 128> data2 = {3.0f};



int main(int input)
{
  d.referTo(data, 64);
  d2.referTo(d, 32, 1);
  
  d2 *= 4.0f;
  
  return (int)data[1];
  
  
}


};

}
namespace vop_test{
namespace vop_5
{

span<float, 128> data = {20.0f};

dyn<float> d;



float main(float input)
{
  d.referTo(data, 64, 3);
  d *= 4.0f;
  
  return data[8];
  
}


};

}
namespace vop_test{
namespace vop_6
{

span<float, 16> data = {1.0f, 2.0f, 3.0f, 4.0f,
                        5.0f, 6.0f, 7.0f, 8.0f,
                        9.0f, 10.0f, 11.0f, 12.0f,
                        13.0f, 14.0f, 15.0f, 16.0f };




float main(float input)
{
	block d;
  block d2;

  	d.referTo(data, 8);
  	d2.referTo(data, 8, 8);
  	d += d2;
  
  	return d[1];
  
}


};

}
namespace vop_test{
namespace vop_7
{

span<float, 16> data = {1.0f, 2.0f, 3.0f, 4.0f,
                        5.0f, 6.0f, 7.0f, 8.0f,
                        9.0f, 10.0f, 11.0f, 12.0f,
                        13.0f, 14.0f, 15.0f, 16.0f };




float main(float input)
{
	block d;
  	block d2;

  	d.referTo(data, 8);
  	d2.referTo(data, 8, 8);
  	d = d2;
  
  	return d[1];
  
}


};

}
namespace wrap_test{
namespace data_set_parameter
{

struct MyClass
{
	DECLARE_NODE(MyClass);

	template <int P> void setParameter(double d)
	{
		value = (int)d;
	}
	

	int value = 8;
};


struct Initialiser
{
	Initialiser(MyClass& d)
	{
		
	}
};


using Wrapped = wrap::init<MyClass, Initialiser>;

using ParameterType = parameter::plain<Wrapped, 0>;

//using CType = container::chain<parameter::plain<MyClass, 0>, MyClass>;




int main(int input)
{
	//CType c;
	
	
	Wrapped obj;
	
	ParameterType p;
	
	p.connect<0>(obj);
	
	obj.setParameter<0>(9.0);
	
	int v1 = obj.getWrappedObject().value;
	
	p.call(2.0);
	
	int v2 = obj.getWrappedObject().value;
	
	return v1 + v2;
}

};

}
namespace wrap_test{
namespace get_self_object
{

struct MyClass
{
	DECLARE_NODE(MyClass);

	template <int P> void setParameter(double d)
	{
		value = (int)d;
	}

	int value = 8;
};


struct Initialiser
{
	Initialiser(MyClass& d)
	{
		d.value = 90;
	}
};

using C1Type = container::chain<parameter::empty, wrap::fix<1, MyClass>>;

using C2Type = container::chain<parameter::empty, wrap::fix<1, wrap::event<MyClass>>>;

using Wrapped = wrap::fix<2, wrap::init<MyClass, Initialiser>>;

using C3Type = container::chain<parameter::empty, Wrapped>;

C1Type c1;
C2Type c2;

Wrapped w;

C3Type c3;

using C4Type = container::chain<parameter::empty, wrap::fix<2, wrap::event<Wrapped>>>;

C4Type c4;

int main(int input)
{
	auto& x = c1.get<0>(); // MyClass
	auto& y = c2.get<0>(); // MyClass
	auto& z = w.getObject(); // wrap::init<MyClass, Initialiser>;
	auto& z2 = w.getWrappedObject(); // MyClass
	auto& t1 = c3.get<0>(); // wrap::init<MyClass, Initialiser>;
	auto& t2 = t1.getObject(); // wrap::init<MyClass, Initialiser>;
	auto& t3 = t2.getWrappedObject(); // MyClass
	auto& t4 = c4.get<0>().getWrappedObject();
	
	return x.value + y.value + t3.value + t4.value;
}

};

}
namespace wrap_test{
namespace simple_wrap_add
{

span<int, 8>::wrapped data = { 13 };


int main(int input)
{
	return data + 2;
}

};

}
namespace wrap_test{
namespace simple_wrap_return
{

span<int, 8>::wrapped data = { 13 };


int main(int input)
{
	return data;
}

};

}
namespace wrap_test{
namespace wrap_data1
{


/** A dummy object that expects a table. */
struct X
{
	DECLARE_NODE(X);

	template <int P> void setParameter(double v) {}
	
	static const int NumTables = 1;

	void setExternalData(const ExternalData& d, int index)
	{
		d.referBlockTo(f, 0);
	}
	
	block f;
};



struct LookupTable
{
	span<float, 19> data = { 182.0f };
};

LookupTable lut;



/** A data handler that forwards the table to the X object. */
struct DataHandler
{
	static const int NumTables = 1;
	static const int NumAudioFiles = 0;
	static const int NumSliderPacks = 0;

	DataHandler(X& obj)
	{
		ExternalData o(lut);
		obj.setExternalData(o, 0);
	}
	
	void setExternalData(X& obj, const ExternalData& d, int index)
	{
		if(index == 0)
		    obj.setExternalData(d, 0);
	}
	
	
};

wrap::data<X, DataHandler> mainObject;



int main(int input)
{
	ExternalData o(lut);

	mainObject.setExternalData(o, 0);

	

	return mainObject.getWrappedObject().f.size();
}

};

}
namespace wrap_test{
namespace wrap_data2
{


/** A dummy object that expects a table. */
struct X
{
	DECLARE_NODE(X);

	static const int NumTables = 1;

	template <int P> void setParameter(double d)
	{
		
	}

	void setExternalData(const ExternalData& d, int index)
	{
		d.referBlockTo(f, 0);
	}
	
	block f;
};



struct LookupTable
{
	span<float, 12> data = { 182.0f };
};

LookupTable lut;



/** A data handler that forwards the table to the X object. */
struct DataHandler
{
	static const int NumTables = 1;
	static const int NumAudioFiles = 0;
	static const int NumSliderPacks = 0;

	DataHandler(X& obj)
	{
		
	}
	
	void setExternalData(X& obj, const ExternalData& d, int index)
	{
		if(index == 0)
		    obj.setExternalData(d, 0);
	}
	
	
};

wrap::data<X, DataHandler> mainObject;



int main(int input)
{
	ExternalData o(lut);
	mainObject.setExternalData(o, 0);
	
	return (int)mainObject.getWrappedObject().f[3];
}

};

}
namespace wrap_test{
namespace wrap_fix
{

struct dc1
{
	DECLARE_NODE(dc1);

	static const int NumChannels = 1;

	template <int P> void setParameter(double d)
	{
		
	}

	void process(ProcessData<1>& d)
	{
		
	}

	template <int C> void processFrame(span<float, C>& data)
	{
		for(auto& s: data)
		    s += 1.0f;
	}
};

struct dc2
{
	DECLARE_NODE(dc2);
	
	static const int NumChannels = 1;

	template <int P> void setParameter(double d)
	{
		
	}

	void process(ProcessData<1>& d)
	{
		
	}

	template <typename D> void processFrame(D& data)
	{
		for(auto& s: data)
			s += 2.0f;
	}
};


wrap::fix<1, dc1> wdc1_obj;

wrap::fix<1, dc2> wdc2_obj;

container::chain<parameter::empty, wrap::fix<1, dc1>> cwdc1_obj;

container::chain<parameter::empty, dc1> cdc1_obj;
container::chain<parameter::empty, dc2, dc1> cdc2_obj;

int main(int input)
{
	span<float, 1> data = {0.0f};
	
	wdc1_obj.processFrame(data);
	wdc2_obj.processFrame(data);
	
	cwdc1_obj.processFrame(data);
	cdc1_obj.processFrame(data);
	cdc2_obj.processFrame(data);
	
	return (int)data[0];
	
}

};

}
namespace wrap_test{
namespace wrap_init1
{

struct X
{
	DECLARE_NODE(X);
	
	template <int P> void setParameter(double v) {};

	X()
	{
		
	}
	
	int value = 90;
};

struct In
{
	In(X& o)
	{
		o.value = 180;
	}
};

wrap::init<X, In> obj;

int main(int input)
{
	return obj.getWrappedObject().value;
}

};

}
namespace wrap_test{
namespace wrap_local_definition
{

int main(int input)
{
	span<int, 5>::wrapped d = input;
	return d;
}

};

}
struct TestFileCppTest : public juce::UnitTest
{
    TestFileCppTest() : juce::UnitTest("TestFileCpp") {};

    void runTest() override
    {
        beginTest("Testing CPP files");

        expectEquals(basic_test::abs_calls::main(-2.f), 5.f, "basic/abs_calls.h");
        expectEquals(basic_test::complex_simple_register::main(12), 8, "basic/complex_simple_register.h");
        expectEquals(basic_test::constant_override::main(12), 5, "basic/constant_override.h");
        expectEquals(basic_test::function_call::main(12), 2, "basic/function_call.h");
        expectEquals(basic_test::function_call_byvalue::main(12), 12, "basic/function_call_byvalue.h");
        expectEquals(basic_test::function_overload::main(12), 9, "basic/function_overload.h");
        expectEquals(basic_test::function_pass_structbyvalue::main(12), 200, "basic/function_pass_structbyvalue.h");
        expectEquals(basic_test::function_ref::main(0), 4, "basic/function_ref.h");
        expectEquals(basic_test::function_ref_local::main(0), 4, "basic/function_ref_local.h");
        expectEquals(basic_test::function_return_ref::main(12), 9, "basic/function_return_ref.h");
        expectEquals(basic_test::function_with_same_parameter_name::main(12), 30, "basic/function_with_same_parameter_name.h");
        expectEquals(basic_test::function_with_ternary::main(12.f), 90.f, "basic/function_with_ternary.h");
        expectEquals(basic_test::global_variable::main(12), 9, "basic/global_variable.h");
        expectEquals(basic_test::if_globalwrite::main(9.f), 1.f, "basic/if_globalwrite.h");
        expectEquals(basic_test::inc_after_cond::main(12), 1, "basic/inc_after_cond.h");
        expectEquals(basic_test::inc_as_var::main(12), 6, "basic/inc_as_var.h");
        expectEquals(basic_test::local_override::main(12), 1, "basic/local_override.h");
        expectEquals(basic_test::local_span_ref::main(12), 9, "basic/local_span_ref.h");
        expectEquals(basic_test::nested_function_call::main(2), 16, "basic/nested_function_call.h");
        expectEquals(basic_test::nested_struct_member_call::main(12), 12, "basic/nested_struct_member_call.h");
        expectEquals(basic_test::pass_struct_ref::main(12), 5, "basic/pass_struct_ref.h");
        expectEquals(basic_test::post_inc::main(12), 9, "basic/post_inc.h");
        expectEquals(basic_test::pre_inc::main(12), 10, "basic/pre_inc.h");
        expectEquals(basic_test::reuse_span_register::main(0.f), 13.f, "basic/reuse_span_register.h");
        expectEquals(basic_test::reuse_struct_member_register::main(12), 12, "basic/reuse_struct_member_register.h");
        expectEquals(basic_test::scoped_local_override::main(12), 1, "basic/scoped_local_override.h");
        expectEquals(basic_test::set_from_other_function::main(12), 6, "basic/set_from_other_function.h");
        expectEquals(basic_test::simplecast::main(12.5f), 12, "basic/simple cast.h");
        expectEquals(basic_test::simplereturn::main(12), 12, "basic/simple return.h");
        expectEquals(basic_test::simplestruct::main(12), 21, "basic/simple struct.h");
        expectEquals(basic_test::simple_if::main(12), 5, "basic/simple_if.h");
        expectEquals(basic_test::span_iteration::main(12), 2, "basic/span_iteration.h");
        expectEquals(basic_test::static_function_call::main(12), 25, "basic/static_function_call.h");
        expectEquals(basic_test::static_function_call2::main(12), 8, "basic/static_function_call2.h");
        expectEquals(basic_test::ternary::main(12), 5, "basic/ternary.h");
        expectEquals(default_arg_test::default_1::main(12), 8, "default_arg/default_1.h");
        expectEquals(default_arg_test::default_2::main(12), 10, "default_arg/default_2.h");
        expectEquals(default_arg_test::default_4::main(12), 17, "default_arg/default_4.h");
        expectEquals(default_arg_test::dyn_size_defaultarg::main(12), 22, "default_arg/dyn_size_defaultarg.h");
        expectEquals(default_arg_test::IndexTypeDefault::main(12), 4, "default_arg/IndexTypeDefault.h");
        expectEquals(dsp_test::delay_funk::main(12), 5, "dsp/delay_funk.h");
        expectEquals(dsp_test::sine_class::main(12), 12, "dsp/sine_class.h");
        expectEquals(dsp_test::slice_1::main(12), 9, "dsp/slice_1.h");
        expectEquals(dsp_test::slice_2::main(12), 9, "dsp/slice_2.h");
        expectEquals(dsp_test::slice_3::main(12), 9, "dsp/slice_3.h");
        expectEquals(dsp_test::slice_4::main(12), 8, "dsp/slice_4.h");
        expectEquals(dsp_test::slice_5::main(12), 9, "dsp/slice_5.h");
        expectEquals(dsp_test::slice_6::main(12), 9, "dsp/slice_6.h");
        expectEquals(dsp_test::slice_pretest::main(24), 19, "dsp/slice_pretest.h");
        expectEquals(dyn_test::dyn2simd::main(12), 36, "dyn/dyn2simd.h");
        expectEquals(dyn_test::dyn_access1::main(12), 2, "dyn/dyn_access1.h");
        expectEquals(dyn_test::dyn_access2::main(6), 4, "dyn/dyn_access2.h");
        expectEquals(dyn_test::dyn_subscript::main(12), 3, "dyn/dyn_subscript.h");
        expectEquals(dyn_test::dyn_wrap_3::main(12.f), 3.f, "dyn/dyn_wrap_3.h");
        expectEquals(dyn_test::global_dyn_with_assignment::main(0), 16, "dyn/global_dyn_with_assignment.h");
        expectEquals(dyn_test::simple_dyn_test::main(0), 16, "dyn/simple_dyn_test.h");
        expectEquals(dyn_test::size_test::main(12), 19, "dyn/size_test.h");
        expectEquals(dyn_test::span_of_dyns::main(0), 48, "dyn/span_of_dyns.h");
        expectEquals(enum_test::enum_as_template_argument::main(12), 9, "enum/enum_as_template_argument.h");
        expectEquals(enum_test::enum_in_struct::main(12), 29, "enum/enum_in_struct.h");
        expectEquals(enum_test::enum_set_parameter::main(12.), 24., "enum/enum_set_parameter.h");
        expectEquals(enum_test::simple_enum::main(12), 1, "enum/simple_enum.h");
        expectEquals(expression_initialiser_test::expression_initialiser::main(-12), 26, "expression_initialiser/expression_initialiser.h");
        expectEquals(expression_initialiser_test::expression_initialiser_nested::main(12), 30, "expression_initialiser/expression_initialiser_nested.h");
        expectEquals(expression_initialiser_test::expression_initialiser_span::main(12), 10, "expression_initialiser/expression_initialiser_span.h");
        expectEquals(expression_initialiser_test::expression_initialiser_span_single::main(12), 26, "expression_initialiser/expression_initialiser_span_single.h");
        expectEquals(expression_initialiser_test::expression_list_span2dyn::main(12), 8, "expression_initialiser/expression_list_span2dyn.h");
        expectEquals(float4_test::float4_basic01::main(12.f), 19.f, "float4/float4_basic01.h");
        expectEquals(float4_test::float4_basic02::main(12.f), 19.f, "float4/float4_basic02.h");
        expectEquals(float4_test::float4_basic03::main(12.f), 90.f, "float4/float4_basic03.h");
        expectEquals(init_test::init_test1::main(12), 12, "init/init_test1.h");
        expectEquals(init_test::init_test10::main(12), 8, "init/init_test10.h");
        expectEquals(init_test::init_test11::main(12), 12, "init/init_test11.h");
        expectEquals(init_test::init_test12::main(12), 12, "init/init_test12.h");
        expectEquals(init_test::init_test13::main(12), 2, "init/init_test13.h");
        expectEquals(init_test::init_test14::main(12), 103, "init/init_test14.h");
        expectEquals(init_test::init_test15::main(12), 1296, "init/init_test15.h");
        expectEquals(init_test::init_test16::main(12), 90, "init/init_test16.h");
        expectEquals(init_test::init_test17::main(12), 24, "init/init_test17.h");
        expectEquals(init_test::init_test18::main(12), 1020, "init/init_test18.h");
        expectEquals(init_test::init_test19::main(12), 20, "init/init_test19.h");
        expectEquals(init_test::init_test2::main(12), 12, "init/init_test2.h");
        expectEquals(init_test::init_test20::main(12), 18, "init/init_test20.h");
        expectEquals(init_test::init_test21::main(12), 18, "init/init_test21.h");
        expectEquals(init_test::init_test22::main(12), 18, "init/init_test22.h");
        expectEquals(init_test::init_test23::main(12), 126, "init/init_test23.h");
        expectEquals(init_test::init_test24::main(12), 340, "init/init_test24.h");
        expectEquals(init_test::init_test3::main(12), 8, "init/init_test3.h");
        expectEquals(init_test::init_test4::main(12), 180, "init/init_test4.h");
        expectEquals(init_test::init_test5::main(12), 180, "init/init_test5.h");
        expectEquals(init_test::init_test6::main(12), 6, "init/init_test6.h");
        expectEquals(init_test::init_test7::main(12), 2, "init/init_test7.h");
        expectEquals(init_test::init_test8::main(12), 12, "init/init_test8.h");
        expectEquals(init_test::init_test9::main(12), 18, "init/init_test9.h");
        expectEquals(loop_test::dyn_simd_1::main(0.f), 1, "loop/dyn_simd_1.h");
        expectEquals(loop_test::loop2simd_1::main(12), 52, "loop/loop2simd_1.h");
        expectEquals(loop_test::loop2simd_2::main(12), 44, "loop/loop2simd_2.h");
        expectEquals(loop_test::loop2simd_3::main(0), 52, "loop/loop2simd_3.h");
        expectEquals(loop_test::loop2simd_4::main(12), 22, "loop/loop2simd_4.h");
        expectEquals(loop_test::loop_combine1::main(12), 2, "loop/loop_combine1.h");
        expectEquals(loop_test::loop_combine2::main(12), 2, "loop/loop_combine2.h");
        expectEquals(loop_test::loop_combine3::main(12), 2, "loop/loop_combine3.h");
        expectEquals(loop_test::loop_combine4::main(12), 2, "loop/loop_combine4.h");
        expectEquals(loop_test::loop_combined9::main(12), 80, "loop/loop_combined9.h");
        expectEquals(loop_test::simdable_non_aligned::main(12), 0, "loop/simdable_non_aligned.h");
        expectEquals(loop_test::unroll_1::main(12), 27, "loop/unroll_1.h");
        expectEquals(loop_test::unroll_2::main(12), 162, "loop/unroll_2.h");
        expectEquals(loop_test::unroll_3::main(12), 22, "loop/unroll_3.h");
        expectEquals(loop_test::unroll_4::main(0), 25, "loop/unroll_4.h");
        expectEquals(loop_test::unroll_5::main(12), 42, "loop/unroll_5.h");
        expectEquals(loop_test::while1::main(0), 12, "loop/while1.h");
        expectEquals(namespace_test::empty_namespace::main(12), 12, "namespace/empty_namespace.h");
        expectEquals(namespace_test::namespaced_struct::main(12), 30, "namespace/namespaced_struct.h");
        expectEquals(namespace_test::namespaced_var::main(12), 5, "namespace/namespaced_var.h");
        expectEquals(namespace_test::nested_namespace::main(12), 12, "namespace/nested_namespace.h");
        expectEquals(namespace_test::other_namespace::main(12), 31, "namespace/other_namespace.h");
        expectEquals(namespace_test::simple_namespace::main(12), 12, "namespace/simple_namespace.h");
        expectEquals(namespace_test::static_struct_member::main(12), 12, "namespace/static_struct_member.h");
        expectEquals(namespace_test::struct_with_same_name::main(12), 23, "namespace/struct_with_same_name.h");
        expectEquals(namespace_test::struct_with_same_name2::main(12), 23, "namespace/struct_with_same_name2.h");
        expectEquals(namespace_test::using_namespace::main(12), 13, "namespace/using_namespace.h");
        expectEquals(node_test::chain_num_channels::main(12), 1, "node/chain_num_channels.h");
        expectEquals(parameter_test::parameter_chain::main(12.), 4., "parameter/parameter_chain.h");
        expectEquals(parameter_test::parameter_expression::main(12.), 9., "parameter/parameter_expression.h");
        expectEquals(parameter_test::parameter_list::main(12.), 7., "parameter/parameter_list.h");
        expectEquals(parameter_test::parameter_mixed::main(12.), 16., "parameter/parameter_mixed.h");
        expectEquals(parameter_test::parameter_range::main(0.5), 110., "parameter/parameter_range.h");
        expectEquals(parameter_test::plain_parameter::main(12.), 82., "parameter/plain_parameter.h");
        expectEquals(parameter_test::plain_parameter_in_chain::main(2.), 2., "parameter/plain_parameter_in_chain.h");
        expectEquals(parameter_test::plain_parameter_wrapped::main(12.), 6., "parameter/plain_parameter_wrapped.h");
        expectEquals(parameter_test::range_macros::main(0.5), 110., "parameter/range_macros.h");
        expectEquals(preprocessor_test::constant_and_macro::main(12), 24, "preprocessor/constant_and_macro.h");
        expectEquals(preprocessor_test::macro_define_type::main(12), 6, "preprocessor/macro_define_type.h");
        expectEquals(preprocessor_test::macro_struct::main(12), 3, "preprocessor/macro_struct.h");
        expectEquals(preprocessor_test::macro_struct_type::main(12), 3, "preprocessor/macro_struct_type.h");
        expectEquals(preprocessor_test::nested_endif::main(12), 12, "preprocessor/nested_endif.h");
        expectEquals(preprocessor_test::nested_macro::main(12), 20, "preprocessor/nested_macro.h");
        expectEquals(preprocessor_test::preprocessor_constant::main(12), 125, "preprocessor/preprocessor_constant.h");
        expectEquals(preprocessor_test::preprocessor_if1::main(12), 190, "preprocessor/preprocessor_if1.h");
        expectEquals(preprocessor_test::preprocessor_if2::main(12), 92, "preprocessor/preprocessor_if2.h");
        expectEquals(preprocessor_test::preprocessor_if4::main(12), 10, "preprocessor/preprocessor_if4.h");
        expectEquals(preprocessor_test::preprocessor_multiline::main(12), 12, "preprocessor/preprocessor_multiline.h");
        expectEquals(preprocessor_test::preprocessor_sum::main(12), 24, "preprocessor/preprocessor_sum.h");
        expectEquals(preprocessor_test::template_macro::main(12), 5, "preprocessor/template_macro.h");
        expectEquals(ra_pass_test::math_noop::main(12), 12, "ra_pass/math_noop.h");
        expectEquals(ra_pass_test::math_noop_double::main(12.), 120., "ra_pass/math_noop_double.h");
        expectEquals(ra_pass_test::math_noop_float::main(12.f), 120.f, "ra_pass/math_noop_float.h");
        expectEquals(ra_pass_test::skip_float::main(12), 19, "ra_pass/skip_float.h");
        expectEquals(ra_pass_test::skip_memory_write::main(12), 12, "ra_pass/skip_memory_write.h");
        expectEquals(ra_pass_test::skip_mov::main(12), 20, "ra_pass/skip_mov.h");
        expectEquals(span_test::add_float4_float5::main(12.f), 1.5f, "span/add_float4_float5.h");
        expectEquals(span_test::chainprocessingofstructs::main(1.f), 0.25f, "span/chain processing of structs.h");
        expectEquals(span_test::float4_as_arg::main(12), 2, "span/float4_as_arg.h");
        expectEquals(span_test::get::main(0), 3, "span/get.h");
        expectEquals(span_test::local_definition::main(0.f), 28.f, "span/local_definition.h");
        expectEquals(span_test::local_definition_scalar::main(0.f), 30.f, "span/local_definition_scalar.h");
        expectEquals(span_test::local_definition_without_alias::main(0.f), 5.f, "span/local_definition_without_alias.h");
        expectEquals(span_test::local_spanandwrap::main(12), 2, "span/local_spanandwrap.h");
        expectEquals(span_test::local_span_anonymous_scope::main(0.f), 2.f, "span/local_span_anonymous_scope.h");
        expectEquals(span_test::local_span_as_function_parameter::main(0), 6, "span/local_span_as_function_parameter.h");
        expectEquals(span_test::loop_break_continue::main(12), 7, "span/loop_break_continue.h");
        expectEquals(span_test::loop_break_continue_nested1::main(12), 15, "span/loop_break_continue_nested1.h");
        expectEquals(span_test::loop_break_continue_nested2::main(12), 16, "span/loop_break_continue_nested2.h");
        expectEquals(span_test::loop_nested::main(12), 30, "span/loop_nested.h");
        expectEquals(span_test::nested_span_copy::main(12), 27, "span/nested_span_copy.h");
        expectEquals(span_test::sequential_write::main(0), 9, "span/sequential_write.h");
        expectEquals(span_test::set::main(12.5f), 4, "span/set.h");
        expectEquals(span_test::simd_add::main(0.f), 36.f, "span/simd_add.h");
        expectEquals(span_test::simd_add_scalar::main(0.f), 52.f, "span/simd_add_scalar.h");
        expectEquals(span_test::simd_dyn_index::main(8), 1, "span/simd_dyn_index.h");
        expectEquals(span_test::simd_mul::main(0.f), 8.f, "span/simd_mul.h");
        expectEquals(span_test::simd_mul_scalar::main(0.f), 8.f, "span/simd_mul_scalar.h");
        expectEquals(span_test::simd_set::main(0.f), 20.f, "span/simd_set.h");
        expectEquals(span_test::simd_set_scalar::main(0.f), 48.f, "span/simd_set_scalar.h");
        expectEquals(span_test::simd_set_self_scalar::main(0.f), 56.f, "span/simd_set_self_scalar.h");
        expectEquals(span_test::simd_sum_self_scalar::main(0.f), 116.f, "span/simd_sum_self_scalar.h");
        expectEquals(span_test::simd_sum_self_scalar_no_opt::main(0.f), 164.f, "span/simd_sum_self_scalar_no_opt.h");
        expectEquals(span_test::slice_test::main(12), 90, "span/slice_test.h");
        expectEquals(span_test::small_object_register::main(12), 3, "span/small_object_register.h");
        expectEquals(span_test::span_1element::main(12), 3, "span/span_1element.h");
        expectEquals(span_test::span_double_index::main(12), 4, "span/span_double_index.h");
        expectEquals(span_test::span_make_index::main(6), 2, "span/span_make_index.h");
        expectEquals(span_test::span_size::main(12), 5, "span/span_size.h");
        expectEquals(span_test::span_with_simplecomplex_type::main(12), 12, "span/span_with_simplecomplex_type.h");
        expectEquals(span_test::to_simd::main(0.f), 12.f, "span/to_simd.h");
        expectEquals(span_test::two_local_spans::main(0.f), 7.f, "span/two_local_spans.h");
        expectEquals(span_test::unsafe_1::main(12), 12, "span/unsafe_1.h");
        expectEquals(span_test::unsafe_3::main(12), 92, "span/unsafe_3.h");
        expectEquals(span_test::unsafe_4::main(12), 100, "span/unsafe_4.h");
        expectEquals(span_test::wrapped_initialiser::main(12), 2, "span/wrapped_initialiser.h");
        expectEquals(span_test::wrap_dec::main(12), 7, "span/wrap_dec.h");
        expectEquals(span_test::wrap_index_simple::main(12), 5, "span/wrap_index_simple.h");
        expectEquals(span_test::wrap_local_def::main(12), 7, "span/wrap_local_def.h");
        expectEquals(span_test::wrap_moved::main(12), 2, "span/wrap_moved.h");
        expectEquals(span_test::wrap_moved1::main(12), 2, "span/wrap_moved1.h");
        expectEquals(span_test::wrap_moved2::main(12), 4, "span/wrap_moved2.h");
        expectEquals(span_test::wrap_moved3::main(12), 4, "span/wrap_moved3.h");
        expectEquals(span_test::wrap_sum::main(12), 17, "span/wrap_sum.h");
        expectEquals(struct_test::basic_copy::main(12), 9, "struct/basic_copy.h");
        expectEquals(struct_test::complex_member_call::main(12), 5, "struct/complex_member_call.h");
        expectEquals(struct_test::double_member_2::main(7), 12, "struct/double_member_2.h");
        expectEquals(struct_test::double_struct_member::main(12), 15, "struct/double_struct_member.h");
        expectEquals(struct_test::inner_struct::main(12), 5, "struct/inner_struct.h");
        expectEquals(struct_test::local_struct_definition::main(0.f), 5.f, "struct/local_struct_definition.h");
        expectEquals(struct_test::local_struct_mask_global::main(0.f), 5.f, "struct/local_struct_mask_global.h");
        expectEquals(struct_test::local_struct_ref::main(12), 27, "struct/local_struct_ref.h");
        expectEquals(struct_test::local_struct_with_default::main(0.f), 5.f, "struct/local_struct_with_default.h");
        expectEquals(struct_test::member_set_function::main(12), 90, "struct/member_set_function.h");
        expectEquals(struct_test::method_arg_override::main(12), 12, "struct/method_arg_override.h");
        expectEquals(struct_test::nested_init_with_constructor::main(3), 31, "struct/nested_init_with_constructor.h");
        expectEquals(struct_test::nested_member_call::main(12), 12, "struct/nested_member_call.h");
        expectEquals(struct_test::simd_on_local_member_span::main(0.f), 9.f, "struct/simd_on_local_member_span.h");
        expectEquals(struct_test::simd_on_member_span::main(0.f), 9.f, "struct/simd_on_member_span.h");
        expectEquals(struct_test::span_as_ref_param::main(0), 6, "struct/span_as_ref_param.h");
        expectEquals(struct_test::span_struct_member_register::main(12), 13, "struct/span_struct_member_register.h");
        expectEquals(struct_test::struct_definition::main(0.f), 5.f, "struct/struct_definition.h");
        expectEquals(struct_test::struct_member_call::main(12), 120, "struct/struct_member_call.h");
        expectEquals(struct_test::struct_ref2function::main(12), 90, "struct/struct_ref2function.h");
        expectEquals(struct_test::sum_reference_func::main(12), 56, "struct/sum_reference_func.h");
        expectEquals(struct_test::two_local_structs::main(0.f), 4.f, "struct/two_local_structs.h");
        expectEquals(template_test::complex_template1::main(12), 8, "template/complex_template1.h");
        expectEquals(template_test::complex_template2::main(12), 8, "template/complex_template2.h");
        expectEquals(template_test::complex_template4::main(12), 9, "template/complex_template4.h");
        expectEquals(template_test::external_template_function::main(12), 3, "template/external_template_function.h");
        expectEquals(template_test::simple_template::main(12), 2, "template/simple_template.h");
        expectEquals(template_test::simple_template10::main(12), 11, "template/simple_template10.h");
        expectEquals(template_test::simple_template13::main(12), 30, "template/simple_template13.h");
        expectEquals(template_test::simple_template14::main(12), 42, "template/simple_template14.h");
        expectEquals(template_test::simple_template15::main(12), 144, "template/simple_template15.h");
        expectEquals(template_test::simple_template16::main(12), 144, "template/simple_template16.h");
        expectEquals(template_test::simple_template17::main(12), 149, "template/simple_template17.h");
        expectEquals(template_test::simple_template18::main(12), 8, "template/simple_template18.h");
        expectEquals(template_test::simple_template19::main(12), 15, "template/simple_template19.h");
        expectEquals(template_test::simple_template2::main(12), 4, "template/simple_template2.h");
        expectEquals(template_test::simple_template20::main(12), 33, "template/simple_template20.h");
        expectEquals(template_test::simple_template21::main(12), 6, "template/simple_template21.h");
        expectEquals(template_test::simple_template22::main(12), 2, "template/simple_template22.h");
        expectEquals(template_test::simple_template23::main(12), 20, "template/simple_template23.h");
        expectEquals(template_test::simple_template24::main(12), 5, "template/simple_template24.h");
        expectEquals(template_test::simple_template25::main(12.), 4., "template/simple_template25.h");
        expectEquals(template_test::simple_template26::main(12.f), 4.5f, "template/simple_template26.h");
        expectEquals(template_test::simple_template27::main(12.f), 4.5f, "template/simple_template27.h");
        expectEquals(template_test::simple_template28::main(12), 120, "template/simple_template28.h");
        expectEquals(template_test::simple_template29::main(12), 8, "template/simple_template29.h");
        expectEquals(template_test::simple_template3::main(12), 4, "template/simple_template3.h");
        expectEquals(template_test::simple_template4::main(12), 24, "template/simple_template4.h");
        expectEquals(template_test::simple_template5::main(12), 36, "template/simple_template5.h");
        expectEquals(template_test::simple_template6::main(12), 18, "template/simple_template6.h");
        expectEquals(template_test::simple_template7::main(12), 21, "template/simple_template7.h");
        expectEquals(template_test::simple_template8::main(12), 14, "template/simple_template8.h");
        expectEquals(template_test::simple_template9::main(12), 4, "template/simple_template9.h");
        expectEquals(template_test::span_in_template::main(12), 2, "template/span_in_template.h");
        expectEquals(template_test::static_function_template::main(12), 5, "template/static_function_template.h");
        expectEquals(template_test::template_overload1::main(12), 21, "template/template_overload1.h");
        expectEquals(template_test::template_static_function::main(12), 9, "template/template_static_function.h");
        expectEquals(template_test::template_static_member::main(12), 9, "template/template_static_member.h");
        expectEquals(template_test::variadic_template1::main(12), 14, "template/variadic_template1.h");
        expectEquals(variadic_test::nested_get::main(12), 9, "variadic/nested_get.h");
        expectEquals(variadic_test::nested_reset::main(12), 2, "variadic/nested_reset.h");
        expectEquals(variadic_test::parse_getter::main(12), 24, "variadic/parse_getter.h");
        expectEquals(variadic_test::prepare_call::main(12), 512, "variadic/prepare_call.h");
        expectEquals(variadic_test::prepare_definition::main(12), 512, "variadic/prepare_definition.h");
        expectEquals(variadic_test::prepare_specs_pass_by_value::main(12), 512, "variadic/prepare_specs_pass_by_value.h");
        expectEquals(variadic_test::process_single_test::main(0.f), 1.25f, "variadic/process_single_test.h");
        expectEquals(variadic_test::process_test::main(12), 112, "variadic/process_test.h");
        expectEquals(variadic_test::reset_method::main(12), 2, "variadic/reset_method.h");
        expectEquals(variadic_test::simple_reset::main(12), 12, "variadic/simple_reset.h");
        expectEquals(variadic_test::simple_variadic_test::main(12), 12, "variadic/simple_variadic_test.h");
        expectEquals(variadic_test::split_test::main(12), 25, "variadic/split_test.h");
        expectEquals(variadic_test::variadic_parser::main(12), 12, "variadic/variadic_parser.h");
        expectEquals(variadic_test::wrap_event::main(12), 12, "variadic/wrap_event.h");
        expectEquals(variadic_test::wrap_event_funky::main(12), 90, "variadic/wrap_event_funky.h");
        expectEquals(visibility_test::private_member_by_method::main(12), 9, "visibility/private_member_by_method.h");
        expectEquals(visibility_test::public_enum::main(12), 10, "visibility/public_enum.h");
        expectEquals(vop_test::vop2::main(12), 12, "vop/vop2.h");
        expectEquals(vop_test::vop_1::main(12), 12, "vop/vop_1.h");
        expectEquals(vop_test::vop_2::main(12.f), 40.f, "vop/vop_2.h");
        expectEquals(vop_test::vop_3::main(12.f), 60.f, "vop/vop_3.h");
        expectEquals(vop_test::vop_4::main(12), 80, "vop/vop_4.h");
        expectEquals(vop_test::vop_5::main(12.f), 80.f, "vop/vop_5.h");
        expectEquals(vop_test::vop_6::main(12.f), 12.f, "vop/vop_6.h");
        expectEquals(vop_test::vop_7::main(12.f), 10.f, "vop/vop_7.h");
        expectEquals(wrap_test::data_set_parameter::main(12), 11, "wrap/data_set_parameter.h");
        expectEquals(wrap_test::get_self_object::main(12), 196, "wrap/get_self_object.h");
        expectEquals(wrap_test::simple_wrap_add::main(12), 7, "wrap/simple_wrap_add.h");
        expectEquals(wrap_test::simple_wrap_return::main(12), 5, "wrap/simple_wrap_return.h");
        expectEquals(wrap_test::wrap_data1::main(12), 19, "wrap/wrap_data1.h");
        expectEquals(wrap_test::wrap_data2::main(12), 182, "wrap/wrap_data2.h");
        expectEquals(wrap_test::wrap_fix::main(12), 8, "wrap/wrap_fix.h");
        expectEquals(wrap_test::wrap_init1::main(12), 180, "wrap/wrap_init1.h");
        expectEquals(wrap_test::wrap_local_definition::main(12), 2, "wrap/wrap_local_definition.h");
    };
};

static TestFileCppTest cppTest;