/**
 *  Value.cpp
 *
 *  Implementation for the Value class, which wraps a PHP userspace 
 *  value (a 'zval' in Zend's terminology) into a C++ object
 *
 *  Reminder for the implementer:
 * 
 *      A 'zval' is an object that represents a _value_ in the PHP user space,
 *      and thus not a variable. A 'value' or 'zval' can be used by many
 *      different variables at the same time. The 'refcount' property of the
 *      zval holds the number of variables ($a, $b, $c, et cetera) that are
 *      all linked to the same value. With this system, PHP can implement copy
 *      on write behavior.
 * 
 *      Next to the refcount, the zval also holds a is_ref property, which is
 *      set to true if all variables linked to the value are references of each
 *      other. Thus is $a, $b and $c all point to the same variable, and is_ref
 *      is set to true, changing the value means that the $a, $b and $c value
 *      are all updated. If is_res was false, a change to $a would not mean a 
 *      change to $b, and the zval should have been copied first.
 * 
 * 
 *  @author Emiel Bruijntjes <emiel.bruijntjes@copernica.com>
 *  @copyright 2013 Copernica BV
 */
#include "includes.h"

/**
 *  Set up namespace
 */
namespace Php {

/**
 *  Constructor (value = NULL)
 */
Value::Value()
{
    // create a null zval
    MAKE_STD_ZVAL(_val);
    ZVAL_NULL(_val);
}

/**
 *  Constructor for null ptr
*/
Value::Value(std::nullptr_t value)
{
    // create a null zval
    MAKE_STD_ZVAL(_val);
    ZVAL_NULL(_val);
}

/**
 *  Constructor based on integer value
 *  @param  value
 */
Value::Value(int16_t value)
{
    // create an integer zval
    MAKE_STD_ZVAL(_val);
    ZVAL_LONG(_val, value);
}

/**
 *  Constructor based on integer value
 *  @param  value
 */
Value::Value(int32_t value)
{
    // create an integer zval
    MAKE_STD_ZVAL(_val);
    ZVAL_LONG(_val, value);
}

/**
 *  Constructor based on long value
 *  @param  value
 */
Value::Value(int64_t value)
{
    // create an integer zval
    MAKE_STD_ZVAL(_val);
    ZVAL_LONG(_val, value);
}

/**
 *  Constructor based on boolean value
 *  @param  value
 */
Value::Value(bool value)
{
    // create a boolean zval
    MAKE_STD_ZVAL(_val);
    ZVAL_BOOL(_val, value);
}

/**
 *  Constructor based on single character
 *  @param  value
 */
Value::Value(char value)
{
    // create a string zval
    MAKE_STD_ZVAL(_val);
    ZVAL_STRINGL(_val, &value, 1, 1);
}

/**
 *  Constructor based on string value
 *  @param  value
 */
Value::Value(const std::string &value)
{
    // create a string zval
    MAKE_STD_ZVAL(_val);
    ZVAL_STRINGL(_val, value.c_str(), value.size(), 1);
}

/**
 *  Constructor based on a byte array
 *  @param  value
 *  @param  size
 */
Value::Value(const char *value, int size)
{
    // create a string zval
    MAKE_STD_ZVAL(_val);
    ZVAL_STRINGL(_val, value, size < 0 ? strlen(value) : size, 1);
}

/**
 *  Constructor based on decimal value
 *  @param  value
 */
Value::Value(double value)
{
    // create a double zval
    MAKE_STD_ZVAL(_val);
    ZVAL_DOUBLE(_val, value);
}

/**
 *  Wrap object around zval
 *  @param  zval        Value to wrap
 *  @param  ref         Force this to be a reference
 */
Value::Value(struct _zval_struct *val, bool ref)
{
    // just copy the zval into this object
    _val = val;
    
    // if the variable is not already a reference, and it has more than one
    // variable pointing to it, we should seperate it so that any changes
    // we're going to make will not change the other variable
    if (ref && Z_REFCOUNT_P(_val) > 1)
    {
        // separate the zval
        SEPARATE_ZVAL_IF_NOT_REF(&_val);
    }
    
    // we see ourselves as reference too
    Z_ADDREF_P(_val);
    
    // we're ready if we do not have to force it as a reference
    if (!ref || Z_ISREF_P(_val)) return;
    
    // make this a reference
    Z_SET_ISREF_P(_val);
}

/**
 *  Copy constructor
 *  @param  value
 */
Value::Value(const Value &that)
{
    // how many references does the other object has?
    if (Z_REFCOUNT_P(that._val) > 1 && !Z_ISREF_P(that._val))
    {
        // there are already multiple variables linked to this value, and it
        // is not a reference. this implies that we can not turn this variable
        // into a reference, otherwise strange things could happen, we're going
        // to create a new zval
        ALLOC_ZVAL(_val);
        INIT_PZVAL_COPY(_val, that._val);
        zval_copy_ctor(_val);
    }
    else
    {
        // simply use the same zval
        _val = that._val;
    }
        
    // the other object only has one variable using it, or it is already
    // a variable by reference, we can safely add one more reference to it
    // and make it a variable by reference if it was not already a ref
    Z_ADDREF_P(_val);

    // make reference
    Z_SET_ISREF_P(_val);
}

/**
 *  Move constructor
 *  @param  value
 */
Value::Value(Value &&that)
{
    // just copy the zval
    _val = that._val;

    // clear the other object
    that._val = nullptr;
}

/**
 *  Destructor
 */
Value::~Value()
{
    // ignore if moved
    if (!_val) return;
    
    // if there were two references or less, we're going to remove a reference
    // and only one reference will remain, the object will then impossible be
    // a reference
    if (Z_REFCOUNT_P(_val) <= 2) Z_UNSET_ISREF_P(_val);
    
    // destruct the zval (this function will decrement the reference counter,
    // and only destruct if there are no other references left)
    zval_ptr_dtor(&_val);
}

/**
 *  Move operator
 *  @param  value
 *  @return Value
 */
Value &Value::operator=(Value &&value)
{
    // skip self assignment
    if (this == &value) return *this;

    // is the object a reference?
    if (Z_ISREF_P(_val))
    {
        // @todo difference if the other object is a reference or not?
        
        // the current object is a reference, this means that we should
        // keep the zval object, and copy the other value into it, get
        // the current refcount
        int refcount = Z_REFCOUNT_P(_val);
        
        // clean up the current zval (but keep the zval structure)
        zval_dtor(_val);
        
        // make the copy
        *_val = *value._val;
        
        // restore reference and refcount setting
        Z_SET_ISREF_TO_P(_val, true);
        Z_SET_REFCOUNT_P(_val, refcount);
        
        // how many references did the old variable have?
        if (Z_REFCOUNT_P(value._val) > 1)
        {
            // the other object already had multiple references, this
            // implies that many other PHP variables are also referring 
            // to it, and we still need to store its contents, with one 
            // reference less
            Z_DELREF_P(value._val);
            
            // and we need to run the copy constructor on the current
            // value, because we're making a deep copy
            zval_copy_ctor(_val);
        }
        else
        {
            // the last and only reference to the other object was
            // removed, we no longer need it
            FREE_ZVAL(value._val);
            
            // the other object is no longer valid
            value._val = nullptr;
        }
    }
    else
    {
        // destruct the zval (this function will decrement the reference counter,
        // and only destruct if there are no other references left)
        zval_ptr_dtor(&_val);

        // just copy the zval completely
        _val = value._val;

        // the other object is no longer valid
        value._val = nullptr;
    }

    // update the object
    return validate();
}

/**
 *  Assignment operator
 *  @param  value
 *  @return Value
 */
Value &Value::operator=(const Value &value)
{
    // skip self assignment
    if (this == &value) return *this;

    // is the object a reference?
    if (Z_ISREF_P(_val))
    {
        // the current object is a reference, this means that we should
        // keep the zval object, and copy the other value into it, get
        // the current refcount
        int refcount = Z_REFCOUNT_P(_val);
        
        // clean up the current zval (but keep the zval structure)
        zval_dtor(_val);
        
        // make the copy
        *_val = *value._val;
        zval_copy_ctor(_val);
        
        // restore refcount and reference setting
        Z_SET_ISREF_TO_P(_val, true);
        Z_SET_REFCOUNT_P(_val, refcount);
    }
    else
    {
        // destruct the zval (this function will decrement the reference counter,
        // and only destruct if there are no other references left)
        zval_ptr_dtor(&_val);

        // just copy the zval, and the refcounter
        _val = value._val;

        // and we have one more reference
        Z_ADDREF_P(_val);
    }
    
    // update the object
    return validate();
}

/**
 *  Assignment operator
 *  @param  value
 *  @return Value
 */
Value &Value::operator=(int16_t value)
{
    // if this is not a reference variable, we should detach it to implement copy on write
    SEPARATE_ZVAL_IF_NOT_REF(&_val);

    // deallocate current zval (without cleaning the zval structure)
    zval_dtor(_val);
    
    // set new value
    ZVAL_LONG(_val, value);

    // update the object
    return validate();
}

/**
 *  Assignment operator
 *  @param  value
 *  @return Value
 */
Value &Value::operator=(int32_t value)
{
    // if this is not a reference variable, we should detach it to implement copy on write
    SEPARATE_ZVAL_IF_NOT_REF(&_val);

    // deallocate current zval (without cleaning the zval structure)
    zval_dtor(_val);
    
    // set new value
    ZVAL_LONG(_val, value);

    // update the object
    return validate();
}

/**
 *  Assignment operator
 *  @param  value
 *  @return Value
 */
Value &Value::operator=(int64_t value)
{
    // if this is not a reference variable, we should detach it to implement copy on write
    SEPARATE_ZVAL_IF_NOT_REF(&_val);

    // deallocate current zval (without cleaning the zval structure)
    zval_dtor(_val);
    
    // set new value
    ZVAL_LONG(_val, value);

    // update the object
    return validate();
}

/**
 *  Assignment operator
 *  @param  value
 *  @return Value
 */
Value &Value::operator=(bool value)
{
    // if this is not a reference variable, we should detach it to implement copy on write
    SEPARATE_ZVAL_IF_NOT_REF(&_val);

    // deallocate current zval (without cleaning the zval structure)
    zval_dtor(_val);
    
    // set new value
    ZVAL_BOOL(_val, value);

    // update the object
    return validate();
}

/**
 *  Assignment operator
 *  @param  value
 *  @return Value
 */
Value &Value::operator=(char value)
{
    // if this is not a reference variable, we should detach it to implement copy on write
    SEPARATE_ZVAL_IF_NOT_REF(&_val);

    // deallocate current zval (without cleaning the zval structure)
    zval_dtor(_val);
    
    // set new value
    ZVAL_STRINGL(_val, &value, 1, 1);
    
    // update the object
    return validate();
}

/**
 *  Assignment operator
 *  @param  value
 *  @return Value
 */
Value &Value::operator=(const std::string &value)
{
    // if this is not a reference variable, we should detach it to implement copy on write
    SEPARATE_ZVAL_IF_NOT_REF(&_val);

    // deallocate current zval (without cleaning the zval structure)
    zval_dtor(_val);
    
    // set new value
    ZVAL_STRINGL(_val, value.c_str(), value.size(), 1);
    
    // update the object
    return validate();
}

/**
 *  Assignment operator
 *  @param  value
 *  @return Value
 */
Value &Value::operator=(const char *value)
{
    // if this is not a reference variable, we should detach it to implement copy on write
    SEPARATE_ZVAL_IF_NOT_REF(&_val);

    // deallocate current zval (without cleaning the zval structure)
    zval_dtor(_val);

    // set new value
    ZVAL_STRING(_val, value, 1);
    
    // update the object
    return validate();
}

/**
 *  Assignment operator
 *  @param  value
 *  @return Value
 */
Value &Value::operator=(double value)
{
    // if this is not a reference variable, we should detach it to implement copy on write
    SEPARATE_ZVAL_IF_NOT_REF(&_val);

    // deallocate current zval (without cleaning the zval structure)
    zval_dtor(_val);
    
    // set new value
    ZVAL_DOUBLE(_val, value);
    
    // update the object
    return validate();
}

/**
 *  Add a value to the object
 *  @param  value
 *  @return Value
 */
Value &Value::operator+=(const Value &value)        { return Arithmetic<std::plus>(this).assign(value); }
Value &Value::operator+=(int16_t value)             { return Arithmetic<std::plus>(this).assign(value); }
Value &Value::operator+=(int32_t value)             { return Arithmetic<std::plus>(this).assign(value); }
Value &Value::operator+=(int64_t value)             { return Arithmetic<std::plus>(this).assign(value); }
Value &Value::operator+=(bool value)                { return Arithmetic<std::plus>(this).assign(value); }
Value &Value::operator+=(char value)                { return Arithmetic<std::plus>(this).assign(value); }
Value &Value::operator+=(const std::string &value)  { return Arithmetic<std::plus>(this).assign(value); }
Value &Value::operator+=(const char *value)         { return Arithmetic<std::plus>(this).assign(value); }
Value &Value::operator+=(double value)              { return Arithmetic<std::plus>(this).assign(value); }

/**
 *  Subtract a value from the object
 *  @param  value
 *  @return Value
 */
Value &Value::operator-=(const Value &value)        { return Arithmetic<std::minus>(this).assign(value); }
Value &Value::operator-=(int16_t value)             { return Arithmetic<std::minus>(this).assign(value); }
Value &Value::operator-=(int32_t value)             { return Arithmetic<std::minus>(this).assign(value); }
Value &Value::operator-=(int64_t value)             { return Arithmetic<std::minus>(this).assign(value); }
Value &Value::operator-=(bool value)                { return Arithmetic<std::minus>(this).assign(value); }
Value &Value::operator-=(char value)                { return Arithmetic<std::minus>(this).assign(value); }
Value &Value::operator-=(const std::string &value)  { return Arithmetic<std::minus>(this).assign(value); }
Value &Value::operator-=(const char *value)         { return Arithmetic<std::minus>(this).assign(value); }
Value &Value::operator-=(double value)              { return Arithmetic<std::minus>(this).assign(value); }

/**
 *  Multiply the object with a certain value
 *  @param  value
 *  @return Value
 */
Value &Value::operator*=(const Value &value)        { return Arithmetic<std::multiplies>(this).assign(value); }
Value &Value::operator*=(int16_t value)             { return Arithmetic<std::multiplies>(this).assign(value); }
Value &Value::operator*=(int32_t value)             { return Arithmetic<std::multiplies>(this).assign(value); }
Value &Value::operator*=(int64_t value)             { return Arithmetic<std::multiplies>(this).assign(value); }
Value &Value::operator*=(bool value)                { return Arithmetic<std::multiplies>(this).assign(value); }
Value &Value::operator*=(char value)                { return Arithmetic<std::multiplies>(this).assign(value); }
Value &Value::operator*=(const std::string &value)  { return Arithmetic<std::multiplies>(this).assign(value); }
Value &Value::operator*=(const char *value)         { return Arithmetic<std::multiplies>(this).assign(value); }
Value &Value::operator*=(double value)              { return Arithmetic<std::multiplies>(this).assign(value); }

/**
 *  Divide the object with a certain value
 *  @param  value
 *  @return Value
 */
Value &Value::operator/=(const Value &value)        { return Arithmetic<std::divides>(this).assign(value); }
Value &Value::operator/=(int16_t value)             { return Arithmetic<std::divides>(this).assign(value); }
Value &Value::operator/=(int32_t value)             { return Arithmetic<std::divides>(this).assign(value); }
Value &Value::operator/=(int64_t value)             { return Arithmetic<std::divides>(this).assign(value); }
Value &Value::operator/=(bool value)                { return Arithmetic<std::divides>(this).assign(value); }
Value &Value::operator/=(char value)                { return Arithmetic<std::divides>(this).assign(value); }
Value &Value::operator/=(const std::string &value)  { return Arithmetic<std::divides>(this).assign(value); }
Value &Value::operator/=(const char *value)         { return Arithmetic<std::divides>(this).assign(value); }
Value &Value::operator/=(double value)              { return Arithmetic<std::divides>(this).assign(value); }

/**
 *  Divide the object with a certain value and get the rest
 *  Note that this does not use the Arithmetic object, because no conversion between floats is necessary
 *  @param  value
 *  @return Value
 */
Value &Value::operator%=(const Value &value)        { return operator=(numericValue() % value.numericValue()); }
Value &Value::operator%=(int16_t value)             { return operator=(numericValue() % value); }
Value &Value::operator%=(int32_t value)             { return operator=(numericValue() % value); }
Value &Value::operator%=(int64_t value)             { return operator=(numericValue() % value); }
Value &Value::operator%=(bool value)                { return operator=(numericValue() % value); }
Value &Value::operator%=(char value)                { return operator=(numericValue() % value); }
Value &Value::operator%=(const std::string &value)  { return operator=(numericValue() % atoi(value.c_str())); }
Value &Value::operator%=(const char *value)         { return operator=(numericValue() % atoi(value)); }
Value &Value::operator%=(double value)              { return operator=(numericValue() % (int)value); }

/**
 *  Assignment operator
 *  @param  value
 *  @return Value
 */
Value Value::operator+(const Value &value)          { return Arithmetic<std::plus>(this).apply(value); }
Value Value::operator+(int16_t value)               { return Arithmetic<std::plus>(this).apply(value); }
Value Value::operator+(int32_t value)               { return Arithmetic<std::plus>(this).apply(value); }
Value Value::operator+(int64_t value)               { return Arithmetic<std::plus>(this).apply(value); }
Value Value::operator+(bool value)                  { return Arithmetic<std::plus>(this).apply(value); }
Value Value::operator+(char value)                  { return Arithmetic<std::plus>(this).apply(value); }
Value Value::operator+(const std::string &value)    { return Arithmetic<std::plus>(this).apply(value); }
Value Value::operator+(const char *value)           { return Arithmetic<std::plus>(this).apply(value); }
Value Value::operator+(double value)                { return Arithmetic<std::plus>(this).apply(value); }

/**
 *  Subtraction operator
 *  @param  value
 *  @return Value
 */
Value Value::operator-(const Value &value)          { return Arithmetic<std::minus>(this).apply(value); }
Value Value::operator-(int16_t value)               { return Arithmetic<std::minus>(this).apply(value); }
Value Value::operator-(int32_t value)               { return Arithmetic<std::minus>(this).apply(value); }
Value Value::operator-(int64_t value)               { return Arithmetic<std::minus>(this).apply(value); }
Value Value::operator-(bool value)                  { return Arithmetic<std::minus>(this).apply(value); }
Value Value::operator-(char value)                  { return Arithmetic<std::minus>(this).apply(value); }
Value Value::operator-(const std::string &value)    { return Arithmetic<std::minus>(this).apply(value); }
Value Value::operator-(const char *value)           { return Arithmetic<std::minus>(this).apply(value); }
Value Value::operator-(double value)                { return Arithmetic<std::minus>(this).apply(value); }

/**
 *  Multiplication operator
 *  @param  value
 *  @return Value
 */
Value Value::operator*(const Value &value)          { return Arithmetic<std::multiplies>(this).apply(value); }
Value Value::operator*(int16_t value)               { return Arithmetic<std::multiplies>(this).apply(value); }
Value Value::operator*(int32_t value)               { return Arithmetic<std::multiplies>(this).apply(value); }
Value Value::operator*(int64_t value)               { return Arithmetic<std::multiplies>(this).apply(value); }
Value Value::operator*(bool value)                  { return Arithmetic<std::multiplies>(this).apply(value); }
Value Value::operator*(char value)                  { return Arithmetic<std::multiplies>(this).apply(value); }
Value Value::operator*(const std::string &value)    { return Arithmetic<std::multiplies>(this).apply(value); }
Value Value::operator*(const char *value)           { return Arithmetic<std::multiplies>(this).apply(value); }
Value Value::operator*(double value)                { return Arithmetic<std::multiplies>(this).apply(value); }

/**
 *  Division operator
 *  @param  value
 *  @return Value
 */
Value Value::operator/(const Value &value)          { return Arithmetic<std::divides>(this).apply(value); }
Value Value::operator/(int16_t value)               { return Arithmetic<std::divides>(this).apply(value); }
Value Value::operator/(int32_t value)               { return Arithmetic<std::divides>(this).apply(value); }
Value Value::operator/(int64_t value)               { return Arithmetic<std::divides>(this).apply(value); }
Value Value::operator/(bool value)                  { return Arithmetic<std::divides>(this).apply(value); }
Value Value::operator/(char value)                  { return Arithmetic<std::divides>(this).apply(value); }
Value Value::operator/(const std::string &value)    { return Arithmetic<std::divides>(this).apply(value); }
Value Value::operator/(const char *value)           { return Arithmetic<std::divides>(this).apply(value); }
Value Value::operator/(double value)                { return Arithmetic<std::divides>(this).apply(value); }

/**
 *  Modulus operator
 *  @param  value
 *  @return Value
 */
Value Value::operator%(const Value &value)          { return Value(numericValue() % value.numericValue()); }
Value Value::operator%(int16_t value)               { return Value(numericValue() % value); }
Value Value::operator%(int32_t value)               { return Value(numericValue() % value); }
Value Value::operator%(int64_t value)               { return Value(numericValue() % value); }
Value Value::operator%(bool value)                  { return Value(numericValue() % value); }
Value Value::operator%(char value)                  { return Value(numericValue() % value); }
Value Value::operator%(const std::string &value)    { return Value(numericValue() % atoi(value.c_str())); }
Value Value::operator%(const char *value)           { return Value(numericValue() % atoi(value)); }
Value Value::operator%(double value)                { return Value(numericValue() % (int)value); }

/**
 *  Call the function in PHP
 *  We have ten variants of this function, depending on the number of parameters
 *  This call operator is only useful when the variable represents a callable
 *  @param  p0-p10          Parameters of the function to be called.
 *  @return Value
 */
Value Value::operator()()
{
    // call with zero parameters
    return exec(0, NULL);
}

/**
 *  Call the function - if the variable holds a callable thing
 *  @param  p0          The first parameter
 *  @return Value
 */
Value Value::operator()(Value p0)
{
    // array of parameters
    zval **params[1] = { &p0._val };

    // call the function
    return exec(1, params);
}

/**
 *  Call the function - if the variable holds a callable thing
 *  @param  p0          The first parameter
 *  @param  p1          The second parameter
 *  @return Value
 */
Value Value::operator()(Value p0, Value p1)
{
    // array of parameters
    zval **params[2] = { &p0._val, &p1._val };

    // call the function
    return exec(2, params);
}

/**
 *  Call the function - if the variable holds a callable thing
 *  @param  p0          The first parameter
 *  @param  p1          The second parameter
 *  @param  p2          The third parameter
 *  @return Value
 */
Value Value::operator()(Value p0, Value p1, Value p2)
{
    // array of parameters
    zval **params[3] = { &p0._val, &p1._val, &p2._val };

    // call the function
    return exec(3, params);
}

/**
 *  Call the function - if the variable holds a callable thing
 *  @param  p0          The first parameter
 *  @param  p1          The second parameter
 *  @param  p2          The third parameter
 *  @param  p3          The fourth parameter
 *  @return Value
 */
Value Value::operator()(Value p0, Value p1, Value p2, Value p3)
{
    // array of parameters
    zval **params[4] = { &p0._val, &p1._val, &p2._val, &p3._val };

    // call the function
    return exec(4, params);
}

/**
 *  Call the function - if the variable holds a callable thing
 *  @param  p0          The first parameter
 *  @param  p1          The second parameter
 *  @param  p2          The third parameter
 *  @param  p3          The fourth parameter
 *  @param  p4          The fifth parameter
 *  @return Value
 */
Value Value::operator()(Value p0, Value p1, Value p2, Value p3, Value p4)
{
    // array of parameters
    zval **params[5] = { &p0._val, &p1._val, &p2._val, &p3._val, &p4._val };

    // call the function
    return exec(5, params);
}

/**
 *  Call the function - if the variable holds a callable thing
 *  @param  p0          The first parameter
 *  @param  p1          The second parameter
 *  @param  p2          The third parameter
 *  @param  p3          The fourth parameter
 *  @param  p4          The fifth parameter
 *  @param  p5          The sixth parameter
 *  @return Value
 */
Value Value::operator()(Value p0, Value p1, Value p2, Value p3, Value p4, Value p5)
{
    // array of parameters
    zval **params[6] = { &p0._val, &p1._val, &p2._val, &p3._val, &p4._val, &p5._val };

    // call the function
    return exec(6, params);
}

/**
 *  Call the function - if the variable holds a callable thing
 *  @param  p0          The first parameter
 *  @param  p1          The second parameter
 *  @param  p2          The third parameter
 *  @param  p3          The fourth parameter
 *  @param  p4          The fifth parameter
 *  @param  p5          The sixth parameter
 *  @param  p6          The seventh parameter
 *  @return Value
 */
Value Value::operator()(Value p0, Value p1, Value p2, Value p3, Value p4, Value p5, Value p6)
{
    // array of parameters
    zval **params[7] = { &p0._val, &p1._val, &p2._val, &p3._val, &p4._val, &p5._val, &p6._val };

    // call the function
    return exec(7, params);
}

/**
 *  Call the function - if the variable holds a callable thing
 *  @param  p0          The first parameter
 *  @param  p1          The second parameter
 *  @param  p2          The third parameter
 *  @param  p3          The fourth parameter
 *  @param  p4          The fifth parameter
 *  @param  p5          The sixth parameter
 *  @param  p6          The seventh parameter
 *  @param  p7          The eighth parameter
 *  @return Value
 */
Value Value::operator()(Value p0, Value p1, Value p2, Value p3, Value p4, Value p5, Value p6, Value p7)
{
    // array of parameters
    zval **params[8] = { &p0._val, &p1._val, &p2._val, &p3._val, &p4._val, &p5._val, &p6._val, &p7._val };

    // call the function
    return exec(8, params);
}

/**
 *  Call the function - if the variable holds a callable thing
 *  @param  p0          The first parameter
 *  @param  p1          The second parameter
 *  @param  p2          The third parameter
 *  @param  p3          The fourth parameter
 *  @param  p4          The fifth parameter
 *  @param  p5          The sixth parameter
 *  @param  p6          The seventh parameter
 *  @param  p7          The eighth parameter
 *  @param  p8          The ninth parameter
 *  @return Value
 */
Value Value::operator()(Value p0, Value p1, Value p2, Value p3, Value p4, Value p5, Value p6, Value p7, Value p8)
{
    // array of parameters
    zval **params[9] = { &p0._val, &p1._val, &p2._val, &p3._val, &p4._val, &p5._val, &p6._val, &p7._val, &p8._val };

    // call the function
    return exec(9, params);
}

/**
 *  Call the function - if the variable holds a callable thing
 *  @param  p0          The first parameter
 *  @param  p1          The second parameter
 *  @param  p2          The third parameter
 *  @param  p3          The fourth parameter
 *  @param  p4          The fifth parameter
 *  @param  p5          The sixth parameter
 *  @param  p6          The seventh parameter
 *  @param  p7          The eighth parameter
 *  @param  p8          The ninth parameter
 *  @param  p9          The tenth parameter
 *  @return Value
 */
Value Value::operator()(Value p0, Value p1, Value p2, Value p3, Value p4, Value p5, Value p6, Value p7, Value p8, Value p9)
{
    // array of parameters
    zval **params[10] = { &p0._val, &p1._val, &p2._val, &p3._val, &p4._val, &p5._val, &p6._val, &p7._val, &p8._val, &p9._val };

    // call the function
    return exec(10, params);
}

/**
 *  Call the method - if the variable holds an object with the given method
 *  @param  name        name of the method to call
 *  @return Value
 */
Value Value::call(const std::string &name)
{
    // call with zero parameters
    return exec(name, 0, NULL);
}

/**
 *  Call the method - if the variable holds an object with the given method
 *  @param  name        name of the method to call
 *  @param  p0          The first parameter
 *  @return Value
 */
Value Value::call(const std::string &name, Value p0)
{
    // array of parameters
    zval **params[] = { &p0._val };

    // call with zero parameters
    return exec(name, 1, params);
}

/**
 *  Call the method - if the variable holds an object with the given method
 *  @param  name        name of the method to call
 *  @param  p0          The first parameter
 *  @param  p1          The second parameter
 *  @return Value
 */
Value Value::call(const std::string &name, Value p0, Value p1)
{
    // array of parameters
    zval **params[] = { &p0._val, &p1._val };

    // call with zero parameters
    return exec(name, 2, params);
}

/**
 *  Call the method - if the variable holds an object with the given method
 *  @param  name        name of the method to call
 *  @param  p0          The first parameter
 *  @param  p1          The second parameter
 *  @param  p2          The third parameter
 *  @return Value
 */
Value Value::call(const std::string &name, Value p0, Value p1, Value p2)
{
    // array of parameters
    zval **params[] = { &p0._val, &p1._val, &p2._val };

    // call with zero parameters
    return exec(name, 3, params);
}

/**
 *  Call the method - if the variable holds an object with the given method
 *  @param  name        name of the method to call
 *  @param  p0          The first parameter
 *  @param  p1          The second parameter
 *  @param  p2          The third parameter
 *  @param  p3          The fourth parameter
 *  @return Value
 */
Value Value::call(const std::string &name, Value p0, Value p1, Value p2, Value p3)
{
    // array of parameters
    zval **params[] = { &p0._val, &p1._val, &p2._val, &p3._val };

    // call with zero parameters
    return exec(name, 4, params);
}

/**
 *  Call the method - if the variable holds an object with the given method
 *  @param  name        name of the method to call
 *  @param  p0          The first parameter
 *  @param  p1          The second parameter
 *  @param  p2          The third parameter
 *  @param  p3          The fourth parameter
 *  @param  p4          The fifth parameter
 *  @return Value
 */
Value Value::call(const std::string &name, Value p0, Value p1, Value p2, Value p3, Value p4)
{
    // array of parameters
    zval **params[] = { &p0._val, &p1._val, &p2._val, &p3._val, &p4._val };

    // call with zero parameters
    return exec(name, 5, params);
}

/**
 *  Call the method - if the variable holds an object with the given method
 *  @param  name        name of the method to call
 *  @param  p0          The first parameter
 *  @param  p1          The second parameter
 *  @param  p2          The third parameter
 *  @param  p3          The fourth parameter
 *  @param  p4          The fifth parameter
 *  @param  p5          The sixth parameter
 *  @return Value
 */
Value Value::call(const std::string &name, Value p0, Value p1, Value p2, Value p3, Value p4, Value p5)
{
    // array of parameters
    zval **params[] = { &p0._val, &p1._val, &p2._val, &p3._val, &p4._val, &p5._val };

    // call with zero parameters
    return exec(name, 6, params);
}

/**
 *  Call the method - if the variable holds an object with the given method
 *  @param  name        name of the method to call
 *  @param  p0          The first parameter
 *  @param  p1          The second parameter
 *  @param  p2          The third parameter
 *  @param  p3          The fourth parameter
 *  @param  p4          The fifth parameter
 *  @param  p5          The sixth parameter
 *  @param  p6          The seventh parameter
 *  @return Value
 */
Value Value::call(const std::string &name, Value p0, Value p1, Value p2, Value p3, Value p4, Value p5, Value p6)
{
    // array of parameters
    zval **params[] = { &p0._val, &p1._val, &p2._val, &p3._val, &p4._val, &p5._val, &p6._val };

    // call with zero parameters
    return exec(name, 7, params);
}

/**
 *  Call the method - if the variable holds an object with the given method
 *  @param  name        name of the method to call
 *  @param  p0          The first parameter
 *  @param  p1          The second parameter
 *  @param  p2          The third parameter
 *  @param  p3          The fourth parameter
 *  @param  p4          The fifth parameter
 *  @param  p5          The sixth parameter
 *  @param  p6          The seventh parameter
 *  @param  p7          The eighth parameter
 *  @return Value
 */
Value Value::call(const std::string &name, Value p0, Value p1, Value p2, Value p3, Value p4, Value p5, Value p6, Value p7)
{
    // array of parameters
    zval **params[] = { &p0._val, &p1._val, &p2._val, &p3._val, &p4._val, &p5._val, &p6._val, &p7._val };

    // call with zero parameters
    return exec(name, 8, params);
}

/**
 *  Call the method - if the variable holds an object with the given method
 *  @param  name        name of the method to call
 *  @param  p0          The first parameter
 *  @param  p1          The second parameter
 *  @param  p2          The third parameter
 *  @param  p3          The fourth parameter
 *  @param  p4          The fifth parameter
 *  @param  p5          The sixth parameter
 *  @param  p6          The seventh parameter
 *  @param  p7          The eighth parameter
 *  @param  p8          The ninth parameter
 *  @return Value
 */
Value Value::call(const std::string &name, Value p0, Value p1, Value p2, Value p3, Value p4, Value p5, Value p6, Value p7, Value p8)
{
    // array of parameters
    zval **params[] = { &p0._val, &p1._val, &p2._val, &p3._val, &p4._val, &p5._val, &p6._val, &p7._val, &p8._val };

    // call with zero parameters
    return exec(name, 9, params);
}

/**
 *  Call the method - if the variable holds an object with the given method
 *  @param  name        name of the method to call
 *  @param  p0          The first parameter
 *  @param  p1          The second parameter
 *  @param  p2          The third parameter
 *  @param  p3          The fourth parameter
 *  @param  p4          The fifth parameter
 *  @param  p5          The sixth parameter
 *  @param  p6          The seventh parameter
 *  @param  p7          The eighth parameter
 *  @param  p8          The ninth parameter
 *  @param  p9          The tenth parameter
 *  @return Value
 */
Value Value::call(const std::string &name, Value p0, Value p1, Value p2, Value p3, Value p4, Value p5, Value p6, Value p7, Value p8, Value p9)
{
    // array of parameters
    zval **params[] = { &p0._val, &p1._val, &p2._val, &p3._val, &p4._val, &p5._val, &p6._val, &p7._val, &p8._val, &p9._val };

    // call with zero parameters
    return exec(name, 10, params);
}

/**
 *  Call function with a number of parameters
 *  @param  argc        Number of parameters
 *  @param  argv        The parameters
 *  @return Value
 */
Value Value::exec(int argc, zval ***params)
{
    // the return zval
    zval *retval = nullptr;

    // the current exception
    zval *oldException = EG(exception);

    // call the function
    if (call_user_function_ex(CG(function_table), NULL, _val, &retval, argc, params, 1, NULL) != SUCCESS) return nullptr;

    // was an exception thrown?
    if (oldException != EG(exception)) throw OrigException(EG(exception));

    // no (additional) exception was thrown
    return retval ? Value(retval) : nullptr;
}

/**
 *  Call method with a number of parameters
 *  @param  name        Name of method to call
 *  @param  argc        Number of parameters
 *  @param  argv        The parameters
 *  @return Value
 */
Value Value::exec(const std::string &name, int argc, struct _zval_struct ***params)
{
    // the method to call and the return value
    zval *method;
    zval *retval;

    // construct the method, this is done similarly to how in PHP we can call
    // a member function using call_user_func: pass in an array with the object
    // and the method to call as the first parameter
    MAKE_STD_ZVAL(method);
    array_init(method);

    // add the object and the method to call
    add_index_zval(method, 0, _val);
    add_index_stringl(method, 1, name.c_str(), name.length(), 0);

    // the current exception
    zval *oldException = EG(exception);

    // call the function
    if (call_user_function_ex(CG(function_table), NULL, method, &retval, argc, params, 1, NULL) != SUCCESS) return nullptr;

    // free the method
    FREE_ZVAL(method);

    // was an exception thrown?
    if (oldException != EG(exception)) throw OrigException(EG(exception));

    // no (additional) exception was thrown
    return retval ? Value(retval) : nullptr;
}

/**
 *  The type of object
 *  @return Type
 */
Type Value::type() const
{
    // return regular type
    return (Type)Z_TYPE_P(_val);
}

/**
 *  Change the internal type
 *  @param  type
 *  @return Value
 */
Value &Value::setType(Type type)
{
    // skip if nothing changes
    if (this->type() == type) return *this;

    // if this is not a reference variable, we should detach it to implement copy on write
    SEPARATE_ZVAL_IF_NOT_REF(&_val);
    
    // run the conversion
    switch (type) {
    case nullType:              convert_to_null(_val); break;
    case numericType:           convert_to_long(_val); break;
    case floatType:             convert_to_double(_val); break;
    case boolType:              convert_to_boolean(_val); break;
    case arrayType:             convert_to_array(_val); break;
    case objectType:            convert_to_object(_val); break;
    case stringType:            convert_to_string(_val); break;
    }
    
    // done
    return *this;
}

/**
 *  Check if the variable holds something that is callable
 *  @return bool
 */ 
bool Value::isCallable() const
{
    // we can not rely on the type, because strings can be callable as well
    return zend_is_callable(_val, 0, NULL);
}   

/**
 *  Make a clone of the type
 *  @return Value
 */
Value Value::clone() const
{
    // the zval that will hold the copy
    zval *copy;
    
    // allocate memory
    ALLOC_ZVAL(copy);
    
    // copy the data
    INIT_PZVAL_COPY(copy, _val);
    
    // run the copy constructor to ensure that everything gets copied
    zval_copy_ctor(copy);
    
    // done
    return Value(copy);
}

/**
 *  Clone the zval to a different type
 *  @param  type
 *  @return Value
 */
Value Value::clone(Type type) const
{
    // regular clone if nothing changes
    if (this->type() == type) return clone();

    // make a clone
    return clone().setType(type);
}

/**
 *  Retrieve the value as integer
 *  @return long
 */
long Value::numericValue() const
{
    // already a long?
    if (isNumeric()) return Z_LVAL_P(_val);
    
    // make a clone
    return clone(numericType).numericValue();
}

/**
 *  Retrieve the value as boolean
 *  @return bool
 */
bool Value::boolValue() const
{
    // already a bool?
    if (isBool()) return Z_BVAL_P(_val);

    // make a clone
    return clone(boolType).boolValue();
}

/**
 *  Retrieve the value as string
 *  @return string
 */
std::string Value::stringValue() const
{
    // already a string?
    if (isString()) return std::string(Z_STRVAL_P(_val), Z_STRLEN_P(_val));

    // make a clone
    return clone(stringType).stringValue();
}

/**
 *  Retrieve raw string value
 *  @return const char *
 */
const char *Value::rawValue() const
{
    // already a string?
    if (isString()) return Z_STRVAL_P(_val);
    
    // make a clone
    return clone(stringType).rawValue();
}

/**
 *  Retrieve the value as decimal
 *  @return double
 */
double Value::floatValue() const
{
    // already a double
    if (isFloat()) return Z_DVAL_P(_val);

    // make a clone
    return clone(floatType).floatValue();
}

/**
 *  The number of members in case of an array or object
 *  @return int
 */
int Value::size() const
{
    // is it an array?
    if (isArray()) 
    {
        // get the number of elements
        return zend_hash_num_elements(Z_ARRVAL_P(_val));
    }

    // or an object?
    else if (isObject())
    {
        // the count_elements member function should be defined
        if (!Z_OBJ_HT_P(_val)->count_elements) return 0;
        
        // create a variable to hold the result
        long result;
        
        // call the function
        return Z_OBJ_HT_P(_val)->count_elements(_val, &result) == SUCCESS ? result : 0;
    }

    // not an array, return string size if this is a string
    else if (isString()) 
    {
        // get string size
        return Z_STRLEN_P(_val);
    }
    
    // in all other situations, we convert the variable to a string
    else
    {
        // make a copy
        Value copy(*this);
        
        // convert the copy to a string
        copy.setType(stringType);
        
        // return the string size
        return copy.size();
    }
}

/**
 *  Does the array contain a certain index?
 *  @param  index
 *  @return bool
 */
bool Value::contains(int index) const
{
    // must be an array
    if (!isArray()) return false;

    // unused variable
    zval **result;
    
    // check if this index is already in the array
    return zend_hash_index_find(Z_ARRVAL_P(_val), index, (void**)&result) != FAILURE;
}

/**
 *  Does the array contain a certain key
 *  @param  key
 *  @param  size
 *  @return boolean
 */
bool Value::contains(const char *key, int size) const
{
    // calculate size
    if (size < 0) size = strlen(key);

    // deal with arrays
    if (isArray())
    {
        // unused variable
        zval **result;
     
        // check if index is already in the array
        return zend_hash_find(Z_ARRVAL_P(_val), key, size+1, (void **)&result) != FAILURE;
    }
    else if (isObject())
    {
        // @todo implementation
        return false;
    }
    else
    {
        // scalar variable
        return false;
    }
}

/**
 *  Get access to a certain array member
 *  @param  index
 *  @return Value
 */
Value Value::get(int index) const
{
    // must be an array
    if (!isArray()) return Value();
    
    // zval to retrieve
    zval **result;
 
    // check if index is in the array
    if (zend_hash_index_find(Z_ARRVAL_P(_val), index, (void **)&result) == FAILURE) return Value();
    
    // wrap the value
    return Value(*result);
}

/**
 *  Get access to a certain assoc member
 *  @param  key
 *  @param  size
 *  @return Value
 */
Value Value::get(const char *key, int size) const
{
    // must be an array
    if (!isArray() && !isObject()) return Value();

    // calculate size
    if (size < 0) size = strlen(key);
    
    // are we in an object or an array?
    if (isArray())
    {
        // the result value
        zval **result;
        
        // check if this index is already in the array, otherwise we return NULL
        if (zend_hash_find(Z_ARRVAL_P(_val), key, size + 1, (void **)&result) == FAILURE) return Value();
        
        // wrap the value
        return Value(*result);
    }
    else
    {
        // @todo implementation for objects
        return Value();
    }
}

/**
 *  Set a certain property
 *  @param  index
 *  @param  value
 *  @return Value
 */
const Value &Value::set(int index, const Value &value)
{
    // the current value
    zval **current;
    
    // check if this index is already in the array, otherwise we return NULL
    if (isArray() && zend_hash_index_find(Z_ARRVAL_P(_val), index, (void **)&current) != FAILURE)
    {
        // skip if nothing is going to change
        if (value._val == *current) return value;
    }

    // must be an array
    setType(arrayType);

    // if this is not a reference variable, we should detach it to implement copy on write
    SEPARATE_ZVAL_IF_NOT_REF(&_val);
    
    // add the value (this will decrement refcount on any current variable)
    add_index_zval(_val, index, value._val);

    // the variable has one more reference (the array entry)
    Z_ADDREF_P(value._val);
    
    // object should stay valid
    validate();
    
    // done
    return value;
}

/**
 *  Set a certain property
 *  @param  key
 *  @param  size
 *  @param  value
 *  @return Value
 */
const Value &Value::set(const char *key, int size, const Value &value)
{
    // the current value
    zval **current;
    
    // check if this index is already in the array, otherwise we return NULL
    if (isArray() && zend_hash_find(Z_ARRVAL_P(_val), key, size + 1, (void **)&current) != FAILURE)
    {
        // skip if nothing is going to change
        if (value._val == *current) return value;
    }

    // must be an array
    setType(arrayType);

    // if this is not a reference variable, we should detach it to implement copy on write
    SEPARATE_ZVAL_IF_NOT_REF(&_val);

    // add the value (this will reduce the refcount of the current value)
    add_assoc_zval_ex(_val, key, size+1, value._val);
    
    // the variable has one more reference (the array entry)
    Z_ADDREF_P(value._val);
    
    // object should stay valid
    validate();
    
    // done
    return value;
}

/**
 *  Array access operator
 *  This can be used for accessing arrays
 *  @param  index
 *  @return HashMember
 */
HashMember<int> Value::operator[](int index) 
{
    return HashMember<int>(this, index);
}

/**
 *  Array access operato
 *  This can be used for accessing associative arrays
 *  @param  key
 *  @return HashMember
 */
HashMember<std::string> Value::operator[](const std::string &key) 
{
    return HashMember<std::string>(this, key);
}

/**
 *  Array access operator
 *  This can be used for accessing associative arrays
 *  @param  key
 *  @return HashMember
 */
HashMember<std::string> Value::operator[](const char *key) 
{
    return HashMember<std::string>(this, key);
}

/**
 *  Custom output stream operator
 *  @param  stream
 *  @param  value
 *  @return ostream
 */
std::ostream &operator<<(std::ostream &stream, const Value &value)
{
    return stream << value.stringValue();
}

/**
 *  End of namespace
 */
}

