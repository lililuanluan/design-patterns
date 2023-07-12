// g++ -std=c++20 -O3 -Wall 4_visitor.cpp -o bin/visitor; bin/visitor
#include <iostream>
#include <vector>
#include <memory>
#include <variant>

/***************Guideline 15： Design for the addition of types or operations****************/
// Point.h
struct Point {
    double x;
    double y;
};
/// there is a limiting choice: whether to extend types or operations
namespace procedural_solution {
    
    
    // 首先有两个形状：圆形，方形，开始的时候只处理两种形状
    enum ShapeType {
        circle,
        square
    };

    class Shape {
    protected:
        explicit Shape(ShapeType type)
            : type_(type)
        {}
    public: 
        virtual ~Shape() = default;  // -- 通过protected构造函数 和 virtual析构函数可以得知，这个shape类用来做基类 
        ShapeType getType() const { return type_; }
    private:
        ShapeType type_;
    };

    // 用这个基类来构造Circle类
    class Circle : public Shape { 
    public: 
        explicit Circle( double radius) 
            : Shape(circle) // -- Circle从Shape的public继承，shape的构造函数是explicit，所以没有default构造函数，所以Circle需要初始化基类
                            // 因为是circle，所以用circle枚举初始化
            , radius_(radius)
        {}
    private:
        double radius_;
        Point center_ {};
    };

    // 我们想要画图形

    // 需要一个draw()函数
    void draw(Circle const& c) {
        std::cout << "draw circle\n"; 
    }

    // 当然不止一个图形，还有Square，主要的不同是构造函数里的枚举是用的square
    // 注意，这里继承的类只需要写相对于基类新加的成员，而不需要写getType()这些已有的
    class Square : public Shape { 
    public: 
        explicit Square( double side) 
            : Shape(square) // -- Circle从Shape的public继承，shape的构造函数是explicit，所以没有default构造函数，所以Circle需要初始化基类
                            // 因为是circle，所以用circle枚举初始化
            , side_(side)
        {}
    private:
        double side_;
        Point center_ {};
    };

    // 同时写一个画Square的函数
    void draw(Square const& c) {
        std::cout << "draw square\n"; 
    }

    // 有了circle和square，现在需要画一个vector of different shapes
    void drawAllShapes(std::vector<std::unique_ptr<Shape>> const& shapes) {
        for( auto const& shape : shapes) { // shape是指针
            switch(shape->getType()) { // 现在shape只是基类的指针，所以需要询问它们各自是什么类型
                case circle:
                    draw(static_cast<Circle const&>(*shape)); // 根据一个枚举成员变量的类型，转换指针的类型，由基类指针转换为子类指针
                    break;
                case square:
                    draw(static_cast<Square const&>(*shape)); 
                    break;
            }
        }
    }

    void first_solution() {
        using Shapes = std::vector<std::unique_ptr<Shape>>;
        Shapes shapes;
        shapes.emplace_back(std::make_unique<Circle>(2.3)); // 在结尾构造
        shapes.emplace_back(std::make_unique<Square>(1.2));
        shapes.emplace_back(std::make_unique<Circle>(4.1)); // make_unique应该是new一个出来，然后返回一个unique_ptr，这里构造是隐式转化为shape的指针
        drawAllShapes(shapes);
    }
    // 现在分析一下这个方案的问题：
    // 太原始，switch对于区分形状不是也给好的选择，而且没有默认值
    // 把形状编码在unscoped enum里面
    // 更深刻的问题：guideline 5：design for extension
    // 如果要加入新的形状：需要改enum，这个改动影响很大，比如要改switch，要改所有从shape继承的类
    // 如果在一个大型项目里，改动一处需要更新很多处，“update marathon”，所以如何能确保把所有该更新的地方都更新了呢

    // 这种explicit handling of types是维护噩梦
    // Scott Meyers:
    // this kind of type-based programming has a long history in C, and one of the things we know about it is that it yields programs that are essentially unmaintainable
    // 我：这种函数和类型分离的写法，为每一个类型就要写一个函数，这样在调用函数的时候就需要判断类型，而OO的写法把函数作为类的成员
}

namespace OO_solution {
    // 如果用OO的写法：划掉枚举类型，在Shape基类添加一个纯虚函数draw()，而不必再记住自己的类型了
    class Shape {    
    public: 
        Shape() = default; // 不需要ShapeType作为成员了，所以构造函数可以default
        virtual ~Shape() = default;  
        virtual void draw() const = 0; // 纯虚，继承的时候必须重写
    };

    // 继承的类只需要实现draw()函数了
    class Circle : public Shape { 
    public: 
        explicit Circle( double radius):             
            radius_(radius)
        {}

        void draw() const override;
    private:
        double radius_;
        Point center_ {};
    };
    void Circle::draw() const {
        std::cout << "draw circle\n";
    }

    class Square : public Shape { 
    public: 
        explicit Square( double side) 
            :  side_(side)
        {}
        void draw() const override;
    private:
        double side_;
        Point center_ {};
    };
    void Square::draw() const {
        std::cout << "draw square\n";
    }

    // 实现完子类的draw()函数后，可以重写drawAllShapes
    void drawAllShapes(std::vector<std::unique_ptr<Shape>> const& shapes) {
        for( auto const& shape : shapes) { 
            // 这里不需要判断shape的类型了
            shape -> draw(); 
        }
    }
    void oo_solution() {
        using Shapes = std::vector<std::unique_ptr<Shape>>;
        Shapes shapes;
        shapes.emplace_back(std::make_unique<Circle>(2.3)); // 在结尾构造
        shapes.emplace_back(std::make_unique<Square>(1.2));
        shapes.emplace_back(std::make_unique<Circle>(4.1)); // make_unique应该是new一个出来，然后返回一个unique_ptr，这里构造是隐式转化为shape的指针
        drawAllShapes(shapes);
    }

    // 但是到此为止，还有缺陷：添加新的shape（Object Oriented）非常简单，但是添加函数operations就很难，需要改动现有代码
    // 比如添加一个serialize()函数，把shape序列化
    // 所以说，目前处理的是closed set of operations，每添加一个虚函数，基类需要改动，所有子类需要改动，即使这个函数不会被调用
    // 但是在procedural solution里面，添加一个operation非常简单

    // OO的优点是固定操作，添加类型非常简单，过程式添加函数很简单，但是添加类型很困难
    // 需要做决策：哪个是open set哪个是closed set
    // 是否可以有两个open set呢？？ 在Acyclic Visitor实现，但是性能有很大损失

}

/***************Guideline 16：Use visitor to extend operations****************/

// OOP对于增加operations的解决方法是visitor pattern
// 考虑在基类增加一个纯虚函数的方法：首先需要为每一个继承的类重写这个函数，而且使增加新的类型变得困难（这是OOP的优势，即每个人都可以容易的增加类型）
// 此外，还可能增加一个常规的虚函数，即这个函数有一个默认的实现，但是默认实现对于一些如serialize的功能并非容易

// 在shape之外，新建一个shapevisitor基类，或许取名为shapeOperation更好，但是沿用传统名字方便沟通
// ShapeVisitor基类为每一个继承类提供一个纯虚函数 visit(xxx)
// 有了这个ShapeVisitor基类，可以容易的添加新的operation，每添加一个operation就是添加一个ShapeVisitor的继承类

// visitor是如何满足开闭原则的：初始的问题是增加操作需要更改Shape基类，这是一个变量，variant point，把这个功能分离出来（单一责任原则），这样就不用频繁改动Shape基类了，只需要增加一个accept纯虚函数

namespace visitor_pattern {
    class Circle;
    class Square;

    class ShapeVisitor {
    public:
        virtual ~ShapeVisitor() = default; // 注意，这里声明为virtual不代表这个函数不存在：虚函数代表有一个默认实现，继承的子类可以不必重写，而纯虚函数表示所有继承的子类都必须重写，析构函数设为虚函数可以让子类定制析构，比如释放一些指针等，这应该是一个惯用法

        // 对每个具体的shape，都写一个visit()函数
        virtual void visit(Circle const&) const = 0;
        virtual void visit(Square const&) const = 0;
        // ...
    };
    
    // 如果想要画形状，需要引入Draw类，继承自ShapeVisitor，而且可以对不同的图形库有不同的类
    class Draw : public ShapeVisitor {
    public: 
        void visit(Circle const& c) const override {
            std::cout << "draw circle\n";
        }
        void visit(Square const& s) const override {
            std::cout << "draw square\n";
        }
        // 对其他具体图形的函数，这相当于一个函数仓库，每个图形自己从仓库里面取自己对应的函数，而这个逻辑是不需要改动的，即使有新的函数添加
        // accept接收的是一个ShapeVisitor基类，但是传入的时候可以传入一个Draw对象，这样每个图形从库里取visit函数的时候取的就是画图的函数
        // 每个图形如何取自己的函数呢：传入*this，就是图形自己，根据重载决议会自动匹配到对应的函数
    };

    class Shape {    
    public: 
        Shape() = default; // 不需要ShapeType作为成员了，所以构造函数可以default
        virtual ~Shape() = default;  
        virtual void accept(ShapeVisitor const& v) const = 0; // 纯虚，继承的时候必须重写
    };

    // 继承的类只需要实现draw()函数了
    class Circle : public Shape { 
    public: 
        explicit Circle( double radius):             
            radius_(radius)
        {}

        // 每个新的形状只需要添加这么一行就可以了
        void accept(ShapeVisitor const& v) const override {
            v.visit(*this);
        }
    private:
        double radius_;
        Point center_ {};
    };   

    class Square : public Shape { 
    public: 
        explicit Square( double side) 
            :  side_(side)
        {}
        void accept(ShapeVisitor const& v) const override {
            v.visit(*this);
        }
    private:
        double side_;
        Point center_ {};
    };

    // 当需要一个操作时，构造一个操作的对象，传入accept函数内
    void drawAllShapes(std::vector<std::unique_ptr<Shape>> const& shapes) {
        for( auto const& shape : shapes) { 
            shape->accept(Draw{});
        }
    }    
    void visitor_solution() {
        using Shapes = std::vector<std::unique_ptr<Shape>>;
        Shapes shapes;
        shapes.emplace_back(std::make_unique<Circle>(2.3)); // 在结尾构造
        shapes.emplace_back(std::make_unique<Square>(1.2));
        shapes.emplace_back(std::make_unique<Circle>(4.1)); // make_unique应该是new一个出来，然后返回一个unique_ptr，这里构造是隐式转化为shape的指针
        drawAllShapes(shapes);
    }

    // 那么这个visitor有什么问题呢：它是一个workaround，不是building on OOP strengths
    // 第一个问题是低的实现灵活性，比如实现一个平移translate visitor，把center加一个offset，这件事对很多图形都是一样的，但是不得不对每个图形实现一遍visit函数
    // 由于基类的限制，无法把这部分逻辑提取出来单独实现一个函数
    // 还有，visit函数本身就被Shape Visitor限制死了，无法改函数参数
    // 第二个问题是，增加新的类型变得困难，新加一个类型需要把所有的visitor家族更新一遍：增加一个基类的纯虚函数，在所有子类实现一遍

    // 问题出现在有循环依赖：shapevisitor依赖所有具体的shape（visit函数），所有具体的shape依赖shape基类，shape基类里面有个accept(ShapeVisitor&) 依赖shapevisitor
    // 因为有这层循环依赖关系，所以添加操作变得容易
    // 经典的visitor叫做Cyclic Visitor
    
    // 第三个问题是visitor的本质上是侵入的，因为每增加一个visitor，都需要增加一个accept函数在shape的基类里面（可能有不同的visitor）// 使用variant可以实现非侵入式的visitor

    // 第四个问题是，在子类里面override了accept函数，但是如果再增加一层继承，而子类可能就没有重写这个函数，但是甚至不会有任何警告产生：因为子类的对象可以被父类的参数接收，当然一个可能的方法是把Circle和Square声明为final，但是这限制了以后的扩展

    // 最后一个问题是，需要查两次虚表，double dispatch，这个不是free的
    
}

namespace std_variant_visitor {
    // 一个variant只能包含多个类型中的一个的对象，而且不应该什么都没有，默认是第一个类型的默认值
    // 一个variant可以存储值，但是不能直接把它赋值给其他如int变量，需要获取值，std::get()可以查询某个类型的值，如果存储的不是查询的类型，则抛出异常
    // 另一种方法是std::get_if()，返回指针，如果没有则返回空指针

    // 第三种是我们关心的，std::visit，可以在存储的变量上执行任何操作，或者说可以传入定制的visitor

    // circle和square都是不需要基类了
    class Circle{
    public:
        explicit Circle( double radius):             
            radius_(radius)
        {}
        // 这里不需要accept了，所以是非侵入式的
    private:
        double radius_;
        Point center_ {};
    };
    class Square { 
    public: 
        explicit Square( double side) 
            :  side_(side)
        {}       
    private:
        double side_;
        Point center_ {};
    };

    using Shape = std::variant<Circle, Square>;
    // 有了variant也不需要指针了，因为指针是要把不同类型的对象放在一个数组里，而有了variant，可以用值语义
    using Shapes = std::vector<Shape>;

    // 对每个类型都实现一个operator()，但是现在不需要基类，所以可以自己决定需要什么样的函数，而不必跟基类一致
    struct Draw {
        void operator()(Circle const& c) const {
            std::cout << "draw circle\n";
        }
        void operator()(Square const& c) const {
            std::cout << "draw square\n";
        }
    };

    // 最后，drawAllShapes函数
    void drawAllShapes(Shapes const& shapes) {
        for(auto const& shape: shapes) 
            std::visit(Draw{}, shape); 
        // std::visit的作用就是type dispatch，如果variant里面是一个circle，调用参数为circle的operator()
        // 手动实现为
        // if(Circle * circle = std::get_if<Circle>(&shape)) {...} else if ... 
    }

    void variant_solution() {
        Shapes shapes;
        shapes.emplace_back(Circle{2.3});
        shapes.emplace_back(Square{1.2});
        shapes.emplace_back(Square{4.1}); // 现在不需要make_unique了
        drawAllShapes(shapes);
    }
}

int main() {

    // procedural_solution::first_solution();
    // OO_solution::oo_solution();
    // visitor_pattern::visitor_solution();
    std_variant_visitor::variant_solution();

    return EXIT_SUCCESS;

}