#ifndef STAN_LANG_GRAMMARS_TERM_GRAMMAR_DEF_HPP
#define STAN_LANG_GRAMMARS_TERM_GRAMMAR_DEF_HPP


#include <stan/lang/ast.hpp>
#include <stan/lang/grammars/expression_grammar.hpp>
#include <stan/lang/grammars/indexes_grammar.hpp>
#include <stan/lang/grammars/term_grammar.hpp>
#include <stan/lang/grammars/whitespace_grammar.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <string>
#include <sstream>
#include <vector>

BOOST_FUSION_ADAPT_STRUCT(stan::lang::index_op,
                          (stan::lang::expression, expr_)
                          (std::vector<std::vector<stan::lang::expression> >,
                           dimss_) )

BOOST_FUSION_ADAPT_STRUCT(stan::lang::index_op_sliced,
                          (stan::lang::expression, expr_)
                          (std::vector<stan::lang::idx>, idxs_) )

BOOST_FUSION_ADAPT_STRUCT(stan::lang::integrate_ode,
                          (std::string, system_function_name_)
                          (stan::lang::expression, y0_)
                          (stan::lang::expression, t0_)
                          (stan::lang::expression, ts_)
                          (stan::lang::expression, theta_)
                          (stan::lang::expression, x_)
                          (stan::lang::expression, x_int_) )

BOOST_FUSION_ADAPT_STRUCT(stan::lang::integrate_ode_cvode,
                          (std::string, system_function_name_)
                          (stan::lang::expression, y0_)
                          (stan::lang::expression, t0_)
                          (stan::lang::expression, ts_)
                          (stan::lang::expression, theta_)
                          (stan::lang::expression, x_)
                          (stan::lang::expression, x_int_)
                          (stan::lang::expression, rel_tol_)
                          (stan::lang::expression, abs_tol_)
                          (stan::lang::expression, max_num_steps_) )

BOOST_FUSION_ADAPT_STRUCT(stan::lang::fun,
                          (std::string, name_)
                          (std::vector<stan::lang::expression>, args_) )

BOOST_FUSION_ADAPT_STRUCT(stan::lang::int_literal,
                          (int, val_)
                          (stan::lang::expr_type, type_))

BOOST_FUSION_ADAPT_STRUCT(stan::lang::double_literal,
                          (double, val_)
                          (stan::lang::expr_type, type_) )


namespace stan {

  namespace lang {

    template <typename Iterator>
    term_grammar<Iterator>::term_grammar(variable_map& var_map,
                                         std::stringstream& error_msgs,
                                         expression_grammar<Iterator>& eg)
      : term_grammar::base_type(term_r),
        var_map_(var_map),
        error_msgs_(error_msgs),
        expression_g(eg),
        indexes_g(var_map, error_msgs, eg) {
      using boost::spirit::qi::_1;
      using boost::spirit::qi::_a;
      using boost::spirit::qi::_b;
      using boost::spirit::qi::char_;
      using boost::spirit::qi::double_;
      using boost::spirit::qi::eps;
      using boost::spirit::qi::int_;
      using boost::spirit::qi::lexeme;
      using boost::spirit::qi::lit;
      using boost::spirit::qi::no_skip;
      using boost::spirit::qi::_pass;
      using boost::spirit::qi::_val;
      using boost::spirit::qi::labels::_r1;

      term_r.name("expression");
      term_r
        = (negated_factor_r(_r1)[set_expression_f(_val, _1)]
            >> *((lit('*') > negated_factor_r(_r1)
                             [multiplication_f(_val, _1,
                                           boost::phoenix::ref(error_msgs_))])
                 | (lit('/') > negated_factor_r(_r1)
                               [division_f(_val, _1,
                                           boost::phoenix::ref(error_msgs_))])
                 | (lit('%') > negated_factor_r(_r1)
                               [modulus_f(_val, _1, _pass,
                                          boost::phoenix::ref(error_msgs_))])
                 | (lit('\\') > negated_factor_r(_r1)
                                [left_division_f(_val, _pass, _1,
                                         boost::phoenix::ref(error_msgs_))])
                 | (lit(".*") > negated_factor_r(_r1)
                                [elt_multiplication_f(_val, _1,
                                          boost::phoenix::ref(error_msgs_))])
                 | (lit("./") > negated_factor_r(_r1)
                                [elt_division_f(_val, _1,
                                        boost::phoenix::ref(error_msgs_))])));

      negated_factor_r.name("expression");
      negated_factor_r
        = lit('-') >> negated_factor_r(_r1)
                      [negate_expr_f(_val, _1, _pass,
                                     boost::phoenix::ref(error_msgs_))]
        | lit('!') >> negated_factor_r(_r1)
                      [logical_negate_expr_f(_val, _1,
                                             boost::phoenix::ref(error_msgs_))]
        | lit('+') >> negated_factor_r(_r1)[set_expression_f(_val, _1)]
        | exponentiated_factor_r(_r1)[set_expression_f(_val, _1)]
        | idx_factor_r(_r1)[set_expression_f(_val, _1)];


      exponentiated_factor_r.name("expression");
      exponentiated_factor_r
        = (idx_factor_r(_r1)[set_expression_f(_val, _1)]
           >> lit('^')
           > negated_factor_r(_r1)
             [exponentiation_f(_val, _1, _r1, _pass,
                               boost::phoenix::ref(error_msgs_))]);

      idx_factor_r.name("expression");
      idx_factor_r
        =  factor_r(_r1)[set_expression_f(_val, _1)]
        > *( ( (+dims_r(_r1))[set_expressionss_f(_a, _1)]
               > eps
               [add_expression_dimss_f(_val, _a, _pass,
                                       boost::phoenix::ref(error_msgs_) )] )
            | (indexes_g(_r1)[set_indexes_f(_b, _1)]
               > eps[add_idxs_f(_val, _b, _pass,
                              boost::phoenix::ref(error_msgs_))])
            | (lit("'")
               > eps[transpose_f(_val, _pass,
                                 boost::phoenix::ref(error_msgs_))]) );

      integrate_ode_r.name("expression");
      integrate_ode_r
        %= (lit("integrate_ode") >> no_skip[!char_("a-zA-Z0-9_")])
        > lit('(')
        > identifier_r          // system function name (function only)
        > lit(',')
        > expression_g(_r1)     // y0
        > lit(',')
        > expression_g(_r1)     // t0 (data only)
        > lit(',')
        > expression_g(_r1)     // ts (data only)
        > lit(',')
        > expression_g(_r1)     // theta
        > lit(',')
        > expression_g(_r1)     // x (data only)
        > lit(',')
        > expression_g(_r1)     // x_int (data only)
        > lit(')') [validate_integrate_ode_f(_val,
                                         boost::phoenix::ref(var_map_),
                                         _pass,
                                         boost::phoenix::ref(error_msgs_))];

      integrate_ode_cvode_r.name("expression");
      integrate_ode_cvode_r
        %= (lit("integrate_ode_cvode") >> no_skip[!char_("a-zA-Z0-9_")])
        > lit('(')
        > identifier_r          // system function name (function only)
        > lit(',')
        > expression_g(_r1)     // y0
        > lit(',')
        > expression_g(_r1)     // t0 (data only)
        > lit(',')
        > expression_g(_r1)     // ts (data only)
        > lit(',')
        > expression_g(_r1)     // theta
        > lit(',')
        > expression_g(_r1)     // x (data only)
        > lit(',')
        > expression_g(_r1)     // x_int (data only)
        > lit(',')
        > expression_g(_r1)     // relative tolerance (data only)
        > lit(',')
        > expression_g(_r1)     // absolute tolerance (data only)
        > lit(',')
        > expression_g(_r1)     // maximum number of steps (data only)
        > lit(')') 
          [validate_integrate_ode_cvode_f(_val, boost::phoenix::ref(var_map_),
                                          _pass,
                                          boost::phoenix::ref(error_msgs_))];
      
      factor_r.name("expression");
      factor_r =
        integrate_ode_r(_r1)[set_expression_f(_val, _1)]
        | integrate_ode_cvode_r(_r1)[set_expression_f(_val, _1)]
        | (fun_r(_r1)[set_fun_f(_b, _1)]
           > eps[set_fun_type_named_f(_val, _b, _r1, _pass,
                                      boost::phoenix::ref(error_msgs_))])
        | (variable_r[set_variable_f(_a, _1)]
           > eps[set_var_type_f(_a, _val, boost::phoenix::ref(var_map_),
                                boost::phoenix::ref(error_msgs_),
                                _pass)])
        | int_literal_r[set_expression_f(_val, _1)]
        | double_literal_r[set_expression_f(_val, _1)]
        | (lit('(')
           > expression_g(_r1)[set_expression_f(_val, _1)]
           > lit(')'));

      int_literal_r.name("integer literal");
      int_literal_r
        %= int_
        >> !(lit('.')
             | lit('e')
             | lit('E'));

      double_literal_r.name("real literal");
      double_literal_r
        %= double_;


      fun_r.name("function and argument expressions");
      fun_r
        %= identifier_r
        >> args_r(_r1);


      identifier_r.name("identifier");
      identifier_r
        %= lexeme[char_("a-zA-Z")
                  >> *char_("a-zA-Z0-9_.")];


      args_r.name("function arguments");
      args_r
        %= (lit('(') >> lit(')'))
        | ((lit('(')
            >> (expression_g(_r1) % ','))
            > lit(')'));

      dim_r.name("array dimension (integer expression)");
      dim_r
        %= expression_g(_r1)
        >> eps[validate_int_expression_f(_val, _pass)];

      dims_r.name("array dimensions");
      dims_r
        %= lit('[')
        >> (dim_r(_r1)
           % ',' )
        >> lit(']');

      variable_r.name("variable name");
      variable_r
        %= identifier_r
        > !lit('(');    // negative lookahead to prevent failure in
                        // fun to try to evaluate as variable [cleaner
                        // error msgs]
    }

  }
}
#endif
