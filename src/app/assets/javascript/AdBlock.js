(function() {
/// Checks if the given string needs ':scope' prepended to it
var addScopeIfNeeded = function(str) {
    var needScope = /^\s*[+>~]/;
    if (needScope.test(str)) {
        str = ':scope ' + str;
    }
    return str;
};
var hideIfHasAsync = async function(subject, target) {
    var nodes = document.querySelectorAll(subject);
    var i = 0;
    for (; i < nodes.length; ++i) {
        if (nodes[i++].nodeName === 'BODY')
            break;
    }
    for (i = 0; i < nodes.length; ++i) {
        let currNode = nodes[i];
        let compStyle = window.getComputedStyle(currNode);
        if (compStyle.cssText && compStyle.cssText.indexOf(target) >= 0) {
            currNode.style.cssText = 'display: none !important;';
        }
    }
};
/// Handles the :has(...) cosmetic filter option
var hideIfHas = function (subject, target) {
    target = addScopeIfNeeded(target);

    if (subject === '*') {
        (async(sub, tar) => {
            hideIfHasAsync(sub, tar);
        })(subject, target);
        return;
    }
    var nodes = document.querySelectorAll(subject), i;
    for (i = 0; i < nodes.length; ++i) {
        if (nodes[i].querySelector(target) !== null) {
            nodes[i].style.cssText = 'display: none !important;';
        }
    }
};
var hideIfNotHasAsync = async function(subject, target) {
    var nodes = document.querySelectorAll(subject);
    var i = 0;
    for (; i < nodes.length; ++i) {
        if (nodes[i++].nodeName === 'BODY')
            break;
    }
    for (; i < nodes.length; ++i) {
        let currNode = nodes[i];
        let compStyle = window.getComputedStyle(currNode);
        if (compStyle.cssText && compStyle.cssText.indexOf(target) >= 0) {
            currNode.style.cssText = 'display: none !important;';
        }
    }
};
/// Handles :if-not(...) cosmetic filter option if it does not have any nested cosmetic filter options
var hideIfNotHas = function (subject, target) {
    target = addScopeIfNeeded(target);

    if (subject === '*') {
        (async(sub, tar) => {
            hideIfNotHasAsync(sub, tar);
        })(subject, target);
        return;
    }
    var nodes = document.querySelectorAll(subject), i;
    for (i = 0; i < nodes.length; ++i) {
        if (nodes[i].querySelector(target) === null) {
            nodes[i].style.cssText = 'display: none !important;';
        }
    }
};
/// Handles the :has-text(...) cosmetic filter option, returns an array of elements that match the given criteria
var hasText = function (selector, text, root) {
    if (root === undefined) {
        root = document;
    }
    var elements = root.querySelectorAll(selector);
    var subElems = [].filter.call(elements, function(element) {
        return RegExp(text).test(element.textContent);
    });
    return subElems;
};
var matchesCSS = function (selector, text, root, pseudoSelector) {
    if (root === undefined)
        root = document;

    if (pseudoSelector === undefined)
        pseudoSelector = null;

    var output = [];
    var nodes = root.querySelectorAll(selector), i, colonIdx, attrName, attrVal;
    if (!nodes || !nodes.length) { return output; }

    colonIdx = text.indexOf(':');
    if (colonIdx < 0) { return output; }
    attrName = text.slice(0, colonIdx).trim();
    attrVal = text.slice(colonIdx + 1).trim();

    for (i = 0; i < nodes.length; ++i) {
        var compStyle = window.getComputedStyle(nodes[i], pseudoSelector);
        if (RegExp(attrVal).test(compStyle[attrName])) {
            output.push(nodes[i]);
        }
    }
    return output;
};
var matchesCSSBefore = function(selector, text, root) {
    return matchesCSS(selector, text, root, ':before');
};
var matchesCSSAfter = function(selector, text, root) {
    return matchesCSS(selector, text, root, ':after');
};
/// Handles the :xpath(...) cosmetic filter option, returns an array of elements that match the given criteria
var doXPath = function (subject, expr, root) {
    if (root === undefined) {
        root = document;
    }
    var output = [], i, j;
    var xpathExpr = document.createExpression(expr, null);
    var xpathResult = null;
    var nodes = root.querySelectorAll(subject), node = null;
    for (i = 0; i < nodes.length; ++i) {
        xpathResult = xpathExpr.evaluate(nodes[i], XPathResult.UNORDERED_NODE_SNAPSHOT_TYPE, xpathResult);
        j = xpathResult.snapshotLength;
        while (j--) {
            node = xpathResult.snapshotItem(j);
            if (node.nodeType === 1) {
                output.push(node);
            }
        }
    }
    return output;
};
/// Similar to the doXPath function, except this traverses the dom tree without the xpath expression evaluations
var nthAncestor = function (subject, expr, root) {
    if (root === undefined)
        root = document;

    var output = [], i, j;
    var nodes = root.querySelectorAll(subject), node = null;

    //expr = nth parent
    for (i = 0; i < nodes.length; ++i) {
        node = nodes[i];
        for (j = 0; j < expr; ++j) {
            node = node.parentElement;
            if (!node)
                break;
        }
        if (node)
            output.push(node);
    }
    return output
};
/// Hides each subject in the document, for which the result of the callback with parameters chainSubject, chainTarget is not null or empty
function hideIfChain(subject, chainSubject, chainTarget, callback) {
    chainSubject = addScopeIfNeeded(chainSubject);

    var nodes = document.querySelectorAll(subject), i;
    for (i = 0; i < nodes.length; ++i) {
        var result = callback(chainSubject, chainTarget, nodes[i]);
        if (result && result.length) {
            nodes[i].style.cssText = 'display: none !important;';
        }
    }
}
/// Hides each subject in the document, for which the result of the callback with parameters chainSubject, chainTarget is null or empty
function hideIfNotChain(subject, chainSubject, chainTarget, callback) {
    chainSubject = addScopeIfNeeded(chainSubject);

    var nodes = document.querySelectorAll(subject), i;
    for (i = 0; i < nodes.length; ++i) {
        var result = callback(chainSubject, chainTarget, nodes[i]);
        if (!result || (result.length < 1)) {
            nodes[i].style.cssText = 'display: none !important;';
        }
    }
}
/// Hides the nodes in the given array
function hideNodes(cb, cbSubj, cbTarget) {
    var nodes;
    if (cbSubj !== undefined) {
        nodes = cb(cbSubj, cbTarget);
    } else {
        nodes = cb;
    }
    if (nodes === null) { return; }
    var i;
    for (i = 0; i < nodes.length; ++i) {
        if (nodes[i] !== null) {
            nodes[i].style.cssText = 'display: none !important;';
        }
    }
}

{{ADBLOCK_INTERNAL}}

})();
