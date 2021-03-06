#ifndef IV_AST_VISITOR_H_
#define IV_AST_VISITOR_H_
#include <iv/detail/type_traits.h>
#include <iv/noncopyable.h>
#include <iv/ast_fwd.h>

namespace iv {
namespace core {
namespace ast {
namespace detail {

template<bool IsConst, typename T>
struct AstVisitorTraits {
};

template<typename T>
struct AstVisitorTraits<true, T> {
  typedef typename std::add_pointer<
      typename std::add_const<T>::type>::type type;
};

template<typename T>
struct AstVisitorTraits<false, T> {
  typedef typename std::add_pointer<T>::type type;
};

}  // namespace iv::core::ast::detail

template<bool IsConst, typename Factory>
class BasicAstVisitor
  : private Noncopyable<BasicAstVisitor<IsConst, Factory> > {
 private:
  template<typename T>
  struct add {
    typedef typename detail::AstVisitorTraits<IsConst, T>::type type;
  };
 public:
  virtual ~BasicAstVisitor() = 0;
  virtual void Visit(typename add<Block<Factory> >::type block) = 0;  //NOLINT
  virtual void Visit(typename add<FunctionStatement<Factory> >::type func) = 0;  //NOLINT
  virtual void Visit(typename add<FunctionDeclaration<Factory> >::type func) = 0;  //NOLINT
  virtual void Visit(typename add<VariableStatement<Factory> >::type var) = 0;  //NOLINT
  virtual void Visit(typename add<EmptyStatement<Factory> >::type empty) = 0;  //NOLINT
  virtual void Visit(typename add<IfStatement<Factory> >::type ifstmt) = 0;  //NOLINT
  virtual void Visit(typename add<DoWhileStatement<Factory> >::type dowhile) = 0;  //NOLINT
  virtual void Visit(typename add<WhileStatement<Factory> >::type whilestmt) = 0;  //NOLINT
  virtual void Visit(typename add<ForStatement<Factory> >::type forstmt) = 0;  //NOLINT
  virtual void Visit(typename add<ForInStatement<Factory> >::type forstmt) = 0;  //NOLINT
  virtual void Visit(typename add<ContinueStatement<Factory> >::type continuestmt) = 0;  //NOLINT
  virtual void Visit(typename add<BreakStatement<Factory> >::type breakstmt) = 0;  //NOLINT
  virtual void Visit(typename add<ReturnStatement<Factory> >::type returnstmt) = 0;  //NOLINT
  virtual void Visit(typename add<WithStatement<Factory> >::type withstmt) = 0;  //NOLINT
  virtual void Visit(typename add<LabelledStatement<Factory> >::type labelledstmt) = 0;  //NOLINT
  virtual void Visit(typename add<SwitchStatement<Factory> >::type switchstmt) = 0;  //NOLINT
  virtual void Visit(typename add<ThrowStatement<Factory> >::type throwstmt) = 0;  //NOLINT
  virtual void Visit(typename add<TryStatement<Factory> >::type trystmt) = 0;  //NOLINT
  virtual void Visit(typename add<DebuggerStatement<Factory> >::type debuggerstmt) = 0;  //NOLINT
  virtual void Visit(typename add<ExpressionStatement<Factory> >::type exprstmt) = 0;  //NOLINT

  virtual void Visit(typename add<Assignment<Factory> >::type assign) = 0;  //NOLINT
  virtual void Visit(typename add<BinaryOperation<Factory> >::type binary) = 0;  //NOLINT
  virtual void Visit(typename add<ConditionalExpression<Factory> >::type cond) = 0;  //NOLINT
  virtual void Visit(typename add<UnaryOperation<Factory> >::type unary) = 0;  //NOLINT
  virtual void Visit(typename add<PostfixExpression<Factory> >::type postfix) = 0;  //NOLINT
  virtual void Visit(typename add<Assigned<Factory> >::type assigned) = 0;  //NOLINT

  virtual void Visit(typename add<StringLiteral<Factory> >::type literal) = 0;  //NOLINT
  virtual void Visit(typename add<NumberLiteral<Factory> >::type literal) = 0;  //NOLINT
  virtual void Visit(typename add<Identifier<Factory> >::type literal) = 0;  //NOLINT
  virtual void Visit(typename add<ThisLiteral<Factory> >::type literal) = 0;  //NOLINT
  virtual void Visit(typename add<NullLiteral<Factory> >::type literal) = 0;  //NOLINT
  virtual void Visit(typename add<TrueLiteral<Factory> >::type literal) = 0;  //NOLINT
  virtual void Visit(typename add<FalseLiteral<Factory> >::type literal) = 0;  //NOLINT
  virtual void Visit(typename add<RegExpLiteral<Factory> >::type literal) = 0;  //NOLINT
  virtual void Visit(typename add<ArrayLiteral<Factory> >::type literal) = 0;  //NOLINT
  virtual void Visit(typename add<ObjectLiteral<Factory> >::type literal) = 0;  //NOLINT
  virtual void Visit(typename add<FunctionLiteral<Factory> >::type literal) = 0;  //NOLINT

  virtual void Visit(typename add<IdentifierAccess<Factory> >::type prop) = 0;  //NOLINT
  virtual void Visit(typename add<IndexAccess<Factory> >::type prop) = 0;  //NOLINT
  virtual void Visit(typename add<FunctionCall<Factory> >::type call) = 0;  //NOLINT
  virtual void Visit(typename add<ConstructorCall<Factory> >::type call) = 0;  //NOLINT

  virtual void Visit(typename add<Declaration<Factory> >::type func) = 0;  //NOLINT
  virtual void Visit(typename add<CaseClause<Factory> >::type func) = 0;  //NOLINT
 protected:
  template<typename T>
  class Acceptor {
   public:
    explicit Acceptor(T* visitor) :  visitor_(visitor) { }
    template<typename U>
    void operator()(U* p) const {
      p->Accept(visitor_);
    }
   private:
    T* const visitor_;
  };

  template<typename T>
  class Visitor {
   public:
    explicit Visitor(T* visitor) :  visitor_(visitor) { }
    template<typename U>
    void operator()(U* p) const {
      visitor_->Visit(p);
    }
   private:
    T* const visitor_;
  };
};

template<bool IsConst, typename Factory>
inline BasicAstVisitor<IsConst, Factory>::~BasicAstVisitor() { }

template<typename Factory>
struct AstVisitor {
  typedef BasicAstVisitor<false, Factory> type;
  typedef BasicAstVisitor<true, Factory> const_type;
};

template<bool IsConst, typename Factory>
class BasicExpressionVisitor
  : private Noncopyable<BasicExpressionVisitor<IsConst, Factory> > {
 private:
  template<typename T>
  struct add {
    typedef typename detail::AstVisitorTraits<IsConst, T>::type type;
  };

 public:
  virtual ~BasicExpressionVisitor() = 0;
  virtual void Visit(typename add<Assignment<Factory> >::type assign) = 0;  //NOLINT
  virtual void Visit(typename add<BinaryOperation<Factory> >::type binary) = 0;  //NOLINT
  virtual void Visit(typename add<ConditionalExpression<Factory> >::type cond) = 0;  //NOLINT
  virtual void Visit(typename add<UnaryOperation<Factory> >::type unary) = 0;  //NOLINT
  virtual void Visit(typename add<PostfixExpression<Factory> >::type postfix) = 0;  //NOLINT

  virtual void Visit(typename add<Assigned<Factory> >::type assigned) = 0;  //NOLINT
  virtual void Visit(typename add<StringLiteral<Factory> >::type literal) = 0;  //NOLINT
  virtual void Visit(typename add<NumberLiteral<Factory> >::type literal) = 0;  //NOLINT
  virtual void Visit(typename add<Identifier<Factory> >::type literal) = 0;  //NOLINT
  virtual void Visit(typename add<ThisLiteral<Factory> >::type literal) = 0;  //NOLINT
  virtual void Visit(typename add<NullLiteral<Factory> >::type literal) = 0;  //NOLINT
  virtual void Visit(typename add<TrueLiteral<Factory> >::type literal) = 0;  //NOLINT
  virtual void Visit(typename add<FalseLiteral<Factory> >::type literal) = 0;  //NOLINT
  virtual void Visit(typename add<RegExpLiteral<Factory> >::type literal) = 0;  //NOLINT
  virtual void Visit(typename add<ArrayLiteral<Factory> >::type literal) = 0;  //NOLINT
  virtual void Visit(typename add<ObjectLiteral<Factory> >::type literal) = 0;  //NOLINT
  virtual void Visit(typename add<FunctionLiteral<Factory> >::type literal) = 0;  //NOLINT

  virtual void Visit(typename add<IdentifierAccess<Factory> >::type prop) = 0;  //NOLINT
  virtual void Visit(typename add<IndexAccess<Factory> >::type prop) = 0;  //NOLINT
  virtual void Visit(typename add<FunctionCall<Factory> >::type call) = 0;  //NOLINT
  virtual void Visit(typename add<ConstructorCall<Factory> >::type call) = 0;  //NOLINT
};

template<bool IsConst, typename Factory>
inline BasicExpressionVisitor<IsConst, Factory>::~BasicExpressionVisitor() { }

template<typename Factory>
struct ExpressionVisitor {
  typedef BasicExpressionVisitor<false, Factory> type;
  typedef BasicExpressionVisitor<true, Factory> const_type;
};

} } }  // namespace iv::core::ast
#endif  // IV_AST_VISITOR_H_
