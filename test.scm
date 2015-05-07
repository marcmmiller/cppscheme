
(import "lib.scm")

;; curried function
(define make-adder
  (lambda (x)
    (lambda (y) (+ x y))))

((make-adder 4) 1)

;; restargs lambda
((lambda (first . rest) rest) 1 2 3 4)

;; begin macro
(begin 5 6 7)

(define *x* (lambda () 5))

(macroify *x*)

(*x*)

;; very basic macro support
(define my-if
  (lambda (_test _t _f)
    (list 'or (list 'and _test _t) _f)))
(macroify my-if)

(my-if #t 5 6)

(my-if #f 6 5)

;; apply builtin function
(apply + '(1 2 3 4))

(apply + 1 2 '(3 4))

(map (lambda (x) (+ 10 x)) '(1 2 3 4))

;; multi-list map
(map + '(0 2 5) '(5 3 0))

;; cond
(let ((a 1)
      (b 2)
      (c 3))
  (cond ((eq? a b) a)
        ((eq? b c) b)
        (else c)))

