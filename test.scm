
(import "lib.scm")

;; curried function
(define make-adder 
  (lambda (x)
    (lambda (y) (+ x y))))

((make-adder 4) 1)

;; restargs lambda
((lambda (first . rest) rest) 1 2 3 4)

(begin 5 6 7)

(define *x* (lambda () 5))

(macroify *x*)

(*x*)

(define if 
  (lambda (_test _t _f)
    (list 'or (list 'and _test _t) _f)))
(macroify if)

(if #t 5 6)

(if #f 6 5)

(apply + '(1 2 3 4))

(apply + 1 2 '(3 4))

(map (lambda (x) (+ 10 x)) '(1 2 3 4))

(map + '(0 2 5) '(5 3 0))
